// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_pipe_handler.h"

#include <fcntl.h>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/sockaddr_storage.h"
#include "net/server/http_connection.h"
#include "net/socket/socket_posix.h"

const int32_t kSendBufferSizeForDevTools = 256 * 1024 * 1024;     // 256Mb
const int32_t kReceiveBufferSizeForDevTools = 100 * 1024 * 1024;  // 100Mb

namespace content {

namespace {

const char kDevToolsHandlerThreadName[] = "DevToolsPipeHandlerThread";

}  // namespace

// PipeWrapper -----------------------------------------------------------------

class PipeWrapper {
 public:
  PipeWrapper(base::WeakPtr<DevToolsPipeHandler> devtools_handler, int handle);
  ~PipeWrapper() = default;
  void Write(const std::string& message);

 private:
  void ReadLoop();
  void OnRead(int result);
  bool HandleReadResult(int result);

  void WriteLoop();
  void OnWrite(int result);
  bool HandleWriteResult(int result);

  void ConnectionClosed();

  scoped_refptr<net::HttpConnection::ReadIOBuffer> read_buffer_;
  scoped_refptr<net::HttpConnection::QueuedWriteIOBuffer> write_buffer_;
  std::unique_ptr<net::SocketPosix> socket_;
  base::WeakPtr<DevToolsPipeHandler> devtools_handler_;

  base::WeakPtrFactory<PipeWrapper> weak_factory_;
};

PipeWrapper::PipeWrapper(base::WeakPtr<DevToolsPipeHandler> devtools_handler,
                         int handle)
    : devtools_handler_(devtools_handler), weak_factory_(this) {
  socket_.reset(new net::SocketPosix());
  net::SockaddrStorage storage;
  socket_->AdoptConnectedSocket(handle, storage);

  read_buffer_ = new net::HttpConnection::ReadIOBuffer();
  read_buffer_->set_max_buffer_size(kReceiveBufferSizeForDevTools);

  write_buffer_ = new net::HttpConnection::QueuedWriteIOBuffer();
  write_buffer_->set_max_buffer_size(kSendBufferSizeForDevTools);

  ReadLoop();
}

void PipeWrapper::ReadLoop() {
  int result;
  while (true) {
    // Increases read buffer size if necessary.
    if (read_buffer_->RemainingCapacity() == 0 &&
        !read_buffer_->IncreaseCapacity()) {
      LOG(ERROR) << "Connection closed, not enough capacity";
      ConnectionClosed();
      return;
    }

    result = socket_->Read(
        read_buffer_.get(), read_buffer_->RemainingCapacity(),
        base::Bind(&PipeWrapper::OnRead, weak_factory_.GetWeakPtr()));
    if (result == net::ERR_IO_PENDING)
      return;
    if (!HandleReadResult(result))
      return;
  }
}

void PipeWrapper::OnRead(int result) {
  if (HandleReadResult(result))
    ReadLoop();
}

bool PipeWrapper::HandleReadResult(int result) {
  if (result < 0) {
    LOG(ERROR) << "Connection terminated while reading from pipe";
    ConnectionClosed();
    return false;
  }

  if (result == 0)
    return true;

  read_buffer_->DidRead(result);

  // Go over the last read chunk, look for \n, extract messages.
  int offset = 0;
  for (int i = read_buffer_->GetSize() - result; i < read_buffer_->GetSize();
       ++i) {
    if (read_buffer_->StartOfBuffer()[i] == '\n') {
      std::string str(read_buffer_->StartOfBuffer() + offset, i - offset);

      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(&DevToolsPipeHandler::HandleMessage, devtools_handler_,
                         str));

      offset = i + 1;
    }
  }
  if (offset)
    read_buffer_->DidConsume(offset);

  return true;
}

void PipeWrapper::Write(const std::string& message) {
  bool writing_in_progress = !write_buffer_->IsEmpty();
  if (write_buffer_->Append(message) && write_buffer_->Append("\n") &&
      !writing_in_progress)
    WriteLoop();
}

void PipeWrapper::WriteLoop() {
  int result = net::OK;
  while (write_buffer_->GetSizeToWrite() > 0) {
    result = socket_->Write(
        write_buffer_.get(), write_buffer_->GetSizeToWrite(),
        base::Bind(&PipeWrapper::OnWrite, weak_factory_.GetWeakPtr()));
    if (result == net::ERR_IO_PENDING || result == net::OK)
      return;
    if (!HandleWriteResult(result))
      break;
  }
}

void PipeWrapper::OnWrite(int result) {
  if (HandleWriteResult(result))
    WriteLoop();
}

bool PipeWrapper::HandleWriteResult(int result) {
  if (result < 0) {
    LOG(ERROR) << "Unable to write into the pipe";
    ConnectionClosed();
    return false;
  }

  write_buffer_->DidConsume(result);
  return true;
}

void PipeWrapper::ConnectionClosed() {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&DevToolsPipeHandler::DetachFromTarget,
                                         devtools_handler_));
}

// DevToolsPipeHandler ---------------------------------------------------

DevToolsPipeHandler::DevToolsPipeHandler(int handle) : weak_factory_(this) {
  browser_target_ = DevToolsAgentHost::CreateForDiscovery();
  browser_target_->AttachClient(this);

  thread_.reset(new base::Thread(kDevToolsHandlerThreadName));
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  if (thread_->StartWithOptions(options)) {
    base::TaskRunner* task_runner = thread_->task_runner().get();
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&DevToolsPipeHandler::StartServerOnHandlerThread,
                       weak_factory_.GetWeakPtr(), thread_.get(), handle));
  }
}

DevToolsPipeHandler::~DevToolsPipeHandler() {
  base::SequencedTaskRunner* task_runner = thread_->task_runner().get();
  task_runner->DeleteSoon(FROM_HERE, pipe_wrapper_.release());

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce([](base::Thread* thread) { delete thread; },
                     thread_.release()));
}

void DevToolsPipeHandler::StartServerOnHandlerThread(base::Thread* thread,
                                                     int handle) {
  DCHECK(thread->task_runner()->BelongsToCurrentThread());
  pipe_wrapper_.reset(new PipeWrapper(weak_factory_.GetWeakPtr(), handle));
}

void DevToolsPipeHandler::HandleMessage(const std::string& message) {
  browser_target_->DispatchProtocolMessage(this, message);
}

void DevToolsPipeHandler::DetachFromTarget() {
  browser_target_->DetachClient(this);
}

void DevToolsPipeHandler::DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                                  const std::string& message) {
  base::TaskRunner* task_runner = thread_->task_runner().get();
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&PipeWrapper::Write, base::Unretained(pipe_wrapper_.get()),
                     message));
}

void DevToolsPipeHandler::AgentHostClosed(DevToolsAgentHost* agent_host,
                                          bool replaced_with_another_client) {}

}  // namespace content
