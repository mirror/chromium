// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_pipe_handler.h"

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
#include "net/server/http_connection.h"
#include "base/threading/thread_restrictions.h"

const int32_t kReceiveBufferSizeForDevTools = 100 * 1024 * 1024;  // 100Mb

namespace content {

namespace {

const char kDevToolsPipeHandlerReadThreadName[] = "kDevToolsPipeHandlerReadThread";
const char kDevToolsPipeHandlerWriteThreadName[] = "DevToolsPipeHandlerWriteThread";

void WriteIntoPipe(int whandle, const std::string& message) {
#if defined(OS_WIN)
  HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(whandle));
#endif

  unsigned total_written = 0;
  while (total_written < message.length()) {
#if defined(OS_WIN)
    DWORD length = message.length() - total_written;
    DWORD result = 0;
    WriteFile(handle, message.data(), length, &result, nullptr);
    fprintf(stderr, "WRITTEN: %d\n", result);
    fflush(stderr);
#else
    int length = message.length() - total_written;
    fprintf(stderr, "WILL WRITE\n");
    int result = write(whandle, message.data() + total_written, length);
    fprintf(stderr, "DID WRITE: %d\n", result);
#endif
    if (!result)
      return;
    total_written += result;
  }
#if defined(OS_WIN)
  DWORD result = 0;
  WriteFile(handle, "\n", 1, &result, nullptr);
#else
  write(whandle, "\n", 1);
#endif
}

}  // namespace

// PipeReader ------------------------------------------------------------------

class PipeReader {
 public:
  PipeReader(base::WeakPtr<DevToolsPipeHandler> devtools_handler, int rhandle);
  ~PipeReader() = default;
  void ReadLoop();

 private:
  bool HandleReadResult(int result);

  void ConnectionClosed();

  scoped_refptr<net::HttpConnection::ReadIOBuffer> read_buffer_;
  base::WeakPtr<DevToolsPipeHandler> devtools_handler_;
#if defined(OS_WIN)
  using Handle = HANDLE;
#else
  using Handle = int;
#endif
  Handle rhandle_;
  base::WeakPtrFactory<PipeReader> weak_factory_;
};

PipeReader::PipeReader(base::WeakPtr<DevToolsPipeHandler> devtools_handler,
                       int rhandle)
    : devtools_handler_(devtools_handler),
      weak_factory_(this) {
#if defined(OS_WIN)
  rhandle_ = reinterpret_cast<Handle>(_get_osfhandle(rhandle));
#else
  rhandle_ = rhandle;
#endif

  read_buffer_ = new net::HttpConnection::ReadIOBuffer();
  read_buffer_->set_max_buffer_size(kReceiveBufferSizeForDevTools);
}

void PipeReader::ReadLoop() {
  while (true) {
    if (read_buffer_->RemainingCapacity() == 0 && !read_buffer_->IncreaseCapacity()) {
      LOG(ERROR) << "Connection closed, not enough capacity";
      ConnectionClosed();
      return;
    }

#if defined(OS_WIN)
    DWORD result = 0;
    ReadFile(rhandle_, read_buffer_->data(), read_buffer_->RemainingCapacity(), &result, nullptr);
#else
    int result = read(rhandle_, read_buffer_->data(), read_buffer_->RemainingCapacity());
#endif

    if (!HandleReadResult(result))
      return;
  }
}

bool PipeReader::HandleReadResult(int result) {
  if (result == 0) {
    LOG(ERROR) << "Connection terminated while reading from pipe";
    ConnectionClosed();
    return false;
  }

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

void PipeReader::ConnectionClosed() {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&DevToolsPipeHandler::DetachFromTarget,
                                         devtools_handler_));
}

// DevToolsPipeHandler ---------------------------------------------------

DevToolsPipeHandler::DevToolsPipeHandler(int rhandle, int whandle) : whandle_(whandle), weak_factory_(this) {
  browser_target_ = DevToolsAgentHost::CreateForDiscovery();
  browser_target_->AttachClient(this);

  read_thread_.reset(new base::Thread(kDevToolsPipeHandlerReadThreadName));
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  if (read_thread_->StartWithOptions(options)) {
    base::TaskRunner* task_runner = read_thread_->task_runner().get();
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&DevToolsPipeHandler::StartReaderOnHandlerThread,
                       weak_factory_.GetWeakPtr(), read_thread_.get(), rhandle));
  }

  write_thread_.reset(new base::Thread(kDevToolsPipeHandlerWriteThreadName));
  write_thread_->StartWithOptions(options);
}

DevToolsPipeHandler::~DevToolsPipeHandler() {
  base::SequencedTaskRunner* task_runner = read_thread_->task_runner().get();
  task_runner->DeleteSoon(FROM_HERE, pipe_reader_.release());

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce([](base::Thread* rthread, base::Thread* wthread) {
        delete rthread;
        delete wthread;
        }, read_thread_.release(), write_thread_.release()));
}

void DevToolsPipeHandler::StartReaderOnHandlerThread(base::Thread* thread,
                                                     int handle) {
  DCHECK(thread->task_runner()->BelongsToCurrentThread());
  pipe_reader_.reset(new PipeReader(weak_factory_.GetWeakPtr(), handle));
  pipe_reader_->ReadLoop();
}

void DevToolsPipeHandler::HandleMessage(const std::string& message) {
  browser_target_->DispatchProtocolMessage(this, message);
}

void DevToolsPipeHandler::DetachFromTarget() {
  browser_target_->DetachClient(this);
}

void DevToolsPipeHandler::DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                                  const std::string& message) {
  base::TaskRunner* task_runner = write_thread_->task_runner().get();
  task_runner->PostTask(FROM_HERE,
      base::BindOnce(base::BindOnce(&WriteIntoPipe, whandle_, std::move(message))));
}

void DevToolsPipeHandler::AgentHostClosed(DevToolsAgentHost* agent_host,
                                          bool replaced_with_another_client) {}

}  // namespace content
