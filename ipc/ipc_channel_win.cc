// Copyright (c) 2012 The Chromium Authors. All rights reserved.	
// Use of this source code is governed by a BSD-style license that can be	
// found in the LICENSE file.	
	
#include "ipc/ipc_channel_win.h"	
	
#include <windows.h>	
#include <stddef.h>	
#include <stdint.h>	
	
#include "base/auto_reset.h"	
#include "base/bind.h"	
#include "base/compiler_specific.h"	
#include "base/logging.h"	
#include "base/memory/ptr_util.h"	
#include "base/pickle.h"	
#include "base/process/process_handle.h"	
#include "base/rand_util.h"	
#include "base/single_thread_task_runner.h"	
#include "base/strings/string_number_conversions.h"	
#include "base/strings/utf_string_conversions.h"	
#include "base/threading/thread_checker.h"	
#include "base/threading/thread_task_runner_handle.h"	
#include "base/win/scoped_handle.h"	
#include "ipc/ipc_listener.h"	
#include "ipc/ipc_logging.h"	
#include "ipc/ipc_message_utils.h"	
	
namespace IPC {	
	
ChannelWin::State::State() = default;	
	
ChannelWin::State::~State() {	
  static_assert(offsetof(ChannelWin::State, context) == 0,	
                "ChannelWin::State should have context as its first data"	
                "member.");	
}	
	
ChannelWin::ChannelWin(Mode mode,	
                       Listener* listener)	
    : ChannelReader(listener),	
      peer_pid_(base::kNullProcessId),	
      waiting_connect_(mode & MODE_SERVER_FLAG),	
      processing_incoming_(false),	
      validate_client_(false),	
      client_secret_(0),	
      weak_factory_(this) {	
  CreatePipe(mode);	
}	
	
ChannelWin::~ChannelWin() {	
  CleanUp();	
  Close();	
}	
	
void ChannelWin::Close() {	
  if (thread_check_.get())	
    DCHECK(thread_check_->CalledOnValidThread());	
	
  if (input_state_.is_pending || output_state_.is_pending)	
    CancelIo(pipe_.Get());	
	
  // Closing the handle at this point prevents us from issuing more requests	
  // form OnIOCompleted().	
  if (pipe_.IsValid())	
    pipe_.Close();	
	
  // Make sure all IO has completed.	
  while (input_state_.is_pending || output_state_.is_pending) {	
    base::MessageLoopForIO::current()->WaitForIOCompletion(INFINITE, this);	
  }	
	
  while (!output_queue_.empty()) {	
    OutputElement* element = output_queue_.front();	
    output_queue_.pop();	
    delete element;	
  }	
}	
	
bool ChannelWin::Send(Message* message) {	
  DCHECK(thread_check_->CalledOnValidThread());	
  DVLOG(2) << "sending message @" << message << " on channel @" << this	
           << " with type " << message->type()	
           << " (" << output_queue_.size() << " in queue)";	
	
  return ProcessMessageForDelivery(message);	
}	
	
bool ChannelWin::ProcessMessageForDelivery(Message* message) {	
	
#ifdef IPC_MESSAGE_LOG_ENABLED	
  Logging::GetInstance()->OnSendMessage(message, "");	
#endif	
	
  TRACE_EVENT_WITH_FLOW0(TRACE_DISABLED_BY_DEFAULT("ipc.flow"),	
                         "ChannelWin::ProcessMessageForDelivery",	
                         message->flags(),	
                         TRACE_EVENT_FLAG_FLOW_OUT);	
	
  // |output_queue_| takes ownership of |message|.	
  OutputElement* element = new OutputElement(message);	
  output_queue_.push(element);	
	
  // ensure waiting to write	
  if (!waiting_connect_) {	
    if (!output_state_.is_pending) {	
      if (!ProcessOutgoingMessages(NULL, 0))	
        return false;	
    }	
  }	
	
  return true;	
}	
	
	
ChannelWin::ReadState ChannelWin::ReadData(	
    char* buffer,	
    int buffer_len,	
    int* /* bytes_read */) {	
  if (!pipe_.IsValid())	
    return READ_FAILED;	
	
  DWORD bytes_read = 0;	
  BOOL ok = ReadFile(pipe_.Get(), buffer, buffer_len,	
                     &bytes_read, &input_state_.context.overlapped);	
  if (!ok) {	
    DWORD err = GetLastError();	
    if (err == ERROR_IO_PENDING) {	
      input_state_.is_pending = true;	
      return READ_PENDING;	
    }	
    LOG(ERROR) << "pipe error: " << err;	
    return READ_FAILED;	
  }	
	
  // We could return READ_SUCCEEDED here. But the way that this code is	
  // structured we instead go back to the message loop. Our completion port	
  // will be signalled even in the "synchronously completed" state.	
  //	
  // This allows us to potentially process some outgoing messages and	
  // interleave other work on this thread when we're getting hammered with	
  // input messages. Potentially, this could be tuned to be more efficient	
  // with some testing.	
  input_state_.is_pending = true;	
  return READ_PENDING;	
}	
	
bool ChannelWin::ShouldDispatchInputMessage(Message* msg) {	
  // Make sure we get a hello when client validation is required.	
  if (validate_client_)	
    return IsHelloMessage(*msg);	
  return true;	
}	
	
void ChannelWin::HandleInternalMessage(const Message& msg) {	
  DCHECK_EQ(msg.type(), static_cast<unsigned>(Channel::HELLO_MESSAGE_TYPE));	
  // The hello message contains one parameter containing the PID.	
  base::PickleIterator it(msg);	
  int32_t claimed_pid;	
  bool failed = !it.ReadInt(&claimed_pid);	
	
  if (!failed && validate_client_) {	
    int32_t secret;	
    failed = it.ReadInt(&secret) ? (secret != client_secret_) : true;	
  }	
	
  if (failed) {	
    NOTREACHED();	
    Close();	
    listener()->OnChannelError();	
    return;	
  }	
	
  peer_pid_ = claimed_pid;	
  // Validation completed.	
  validate_client_ = false;	
	
  listener()->OnChannelConnected(claimed_pid);	
}	
	
bool ChannelWin::DidEmptyInputBuffers() {	
  // We don't need to do anything here.	
  return true;	
}	
	
// static	
const base::string16 ChannelWin::PipeName(const std::string& channel_id,	
                                          int32_t* secret) {	
  std::string name("\\\\.\\pipe\\chrome.");	
	
  // Prevent the shared secret from ending up in the pipe name.	
  size_t index = channel_id.find_first_of('\\');	
  if (index != std::string::npos) {	
    if (secret)  // Retrieve the secret if asked for.	
      base::StringToInt(channel_id.substr(index + 1), secret);	
    return base::ASCIIToUTF16(name.append(channel_id.substr(0, index - 1)));	
  }	
	
  // This case is here to support predictable named pipes in tests.	
  if (secret)	
    *secret = 0;	
  return base::ASCIIToUTF16(name.append(channel_id));	
}	
	
bool ChannelWin::CreatePipe(Mode mode) {	
  DCHECK(!pipe_.IsValid());	
  base::string16 pipe_name;	
  if (mode & MODE_SERVER_FLAG) {	
    const DWORD open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |	
                            FILE_FLAG_FIRST_PIPE_INSTANCE;	
    pipe_name = PipeName("chrometempipcpipe", &client_secret_);	
    validate_client_ = !!client_secret_;	
    pipe_.Set(CreateNamedPipeW(pipe_name.c_str(),	
                               open_mode,	
                               PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,	
                               1,	
                               Channel::kReadBufferSize,	
                               Channel::kReadBufferSize,	
                               5000,	
                               NULL));	
  } else if (mode & MODE_CLIENT_FLAG) {	
    pipe_name = PipeName("chrometempipcpipe", &client_secret_);	
    pipe_.Set(CreateFileW(pipe_name.c_str(),	
                          GENERIC_READ | GENERIC_WRITE,	
                          0,	
                          NULL,	
                          OPEN_EXISTING,	
                          SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS |	
                              FILE_FLAG_OVERLAPPED,	
                          NULL));	
  } else {	
    NOTREACHED();	
  }	
	
  if (!pipe_.IsValid()) {	
    // If this process is being closed, the pipe may be gone already.	
    PLOG(WARNING) << "Unable to create pipe \"" << pipe_name << "\" in "	
                  << (mode & MODE_SERVER_FLAG ? "server" : "client") << " mode";	
    return false;	
  }	
	
  // Create the Hello message to be sent when Connect is called	
  std::unique_ptr<Message> m(new Message(MSG_ROUTING_NONE, HELLO_MESSAGE_TYPE,	
                                         IPC::Message::PRIORITY_NORMAL));	
	
  // Don't send the secret to the untrusted process, and don't send a secret	
  // if the value is zero (for IPC backwards compatability).	
  int32_t secret = validate_client_ ? 0 : client_secret_;	
  if (!m->WriteInt(GetCurrentProcessId()) ||	
      (secret && !m->WriteUInt32(secret))) {	
    pipe_.Close();	
    return false;	
  }	
	
  OutputElement* element = new OutputElement(m.release());	
  output_queue_.push(element);	
  return true;	
}	
	
bool ChannelWin::Connect() {	
  DCHECK(base::MessageLoopForIO::IsCurrent());	
	
  WillConnect();	
	
  DLOG_IF(WARNING, thread_check_.get()) << "Connect called more than once";	
	
  if (!thread_check_.get())	
    thread_check_.reset(new base::ThreadChecker());	
	
  if (!pipe_.IsValid())	
    return false;	
	
  base::MessageLoopForIO::current()->RegisterIOHandler(pipe_.Get(), this);	
	
  // Check to see if there is a client connected to our pipe...	
  if (waiting_connect_)	
    ProcessConnection();	
	
  if (!input_state_.is_pending) {	
    // Complete setup asynchronously. By not setting input_state_.is_pending	
    // to true, we indicate to OnIOCompleted that this is the special	
    // initialization signal.	
    base::ThreadTaskRunnerHandle::Get()->PostTask(	
        FROM_HERE,	
        base::Bind(&ChannelWin::OnIOCompleted, weak_factory_.GetWeakPtr(),	
                   &input_state_.context, 0, 0));	
  }	
	
  if (!waiting_connect_)	
    ProcessOutgoingMessages(NULL, 0);	
  return true;	
}	
	
bool ChannelWin::ProcessConnection() {	
  DCHECK(thread_check_->CalledOnValidThread());	
  if (input_state_.is_pending)	
    input_state_.is_pending = false;	
	
  // Do we have a client connected to our pipe?	
  if (!pipe_.IsValid())	
    return false;	
	
  BOOL ok = ConnectNamedPipe(pipe_.Get(), &input_state_.context.overlapped);	
  DWORD err = GetLastError();	
  if (ok) {	
    // Uhm, the API documentation says that this function should never	
    // return success when used in overlapped mode.	
    NOTREACHED();	
    return false;	
  }	
	
  switch (err) {	
  case ERROR_IO_PENDING:	
    input_state_.is_pending = true;	
    break;	
  case ERROR_PIPE_CONNECTED:	
    waiting_connect_ = false;	
    break;	
  case ERROR_NO_DATA:	
    // The pipe is being closed.	
    return false;	
  default:	
    NOTREACHED();	
    return false;	
  }	
	
  return true;	
}	
	
bool ChannelWin::ProcessOutgoingMessages(	
    base::MessageLoopForIO::IOContext* context,	
    DWORD bytes_written) {	
  DCHECK(!waiting_connect_);  // Why are we trying to send messages if there's	
                              // no connection?	
  DCHECK(thread_check_->CalledOnValidThread());	
	
  if (output_state_.is_pending) {	
    DCHECK(context);	
    output_state_.is_pending = false;	
    if (!context || bytes_written == 0) {	
      DWORD err = GetLastError();	
      LOG(ERROR) << "pipe error: " << err;	
      return false;	
    }	
    // Message was sent.	
    CHECK(!output_queue_.empty());	
    OutputElement* element = output_queue_.front();	
    output_queue_.pop();	
    delete element;	
  }	
	
  if (output_queue_.empty())	
    return true;	
	
  if (!pipe_.IsValid())	
    return false;	
	
  // Write to pipe...	
  OutputElement* element = output_queue_.front();	
  DCHECK(element->size() <= INT_MAX);	
  BOOL ok = WriteFile(pipe_.Get(),	
                      element->data(),	
                      static_cast<uint32_t>(element->size()),	
                      NULL,	
                      &output_state_.context.overlapped);	
  if (!ok) {	
    DWORD write_error = GetLastError();	
    if (write_error == ERROR_IO_PENDING) {	
      output_state_.is_pending = true;	
	
      const Message* m = element->get_message();	
      if (m) {	
        DVLOG(2) << "sent pending message @" << m << " on channel @" << this	
                 << " with type " << m->type();	
      }	
	
      return true;	
    }	
    LOG(ERROR) << "pipe error: " << write_error;	
    return false;	
  }	
	
  const Message* m = element->get_message();	
  if (m) {	
    DVLOG(2) << "sent message @" << m << " on channel @" << this	
             << " with type " << m->type();	
  }	
	
  output_state_.is_pending = true;	
  return true;	
}	
	
void ChannelWin::OnIOCompleted(	
    base::MessageLoopForIO::IOContext* context,	
    DWORD bytes_transfered,	
    DWORD error) {	
  bool ok = true;	
  DCHECK(thread_check_->CalledOnValidThread());	
  if (context == &input_state_.context) {	
    if (waiting_connect_) {	
      if (!ProcessConnection())	
        return;	
      // We may have some messages queued up to send...	
      if (!output_queue_.empty() && !output_state_.is_pending)	
        ProcessOutgoingMessages(NULL, 0);	
      if (input_state_.is_pending)	
        return;	
      // else, fall-through and look for incoming messages...	
    }	
	
    // We don't support recursion through OnMessageReceived yet!	
    DCHECK(!processing_incoming_);	
    base::AutoReset<bool> auto_reset_processing_incoming(	
        &processing_incoming_, true);	
	
    // Process the new data.	
    if (input_state_.is_pending) {	
      // This is the normal case for everything except the initialization step.	
      input_state_.is_pending = false;	
      if (!bytes_transfered) {	
        ok = false;	
      } else if (pipe_.IsValid()) {	
        ok = (AsyncReadComplete(bytes_transfered) != DISPATCH_ERROR);	
      }	
    } else {	
      DCHECK(!bytes_transfered);	
    }	
	
    // Request more data.	
    if (ok)	
      ok = (ProcessIncomingMessages() != DISPATCH_ERROR);	
  } else {	
    DCHECK(context == &output_state_.context);	
    CHECK(output_state_.is_pending);	
    ok = ProcessOutgoingMessages(context, bytes_transfered);	
  }	
  if (!ok && pipe_.IsValid()) {	
    // We don't want to re-enter Close().	
    Close();	
    listener()->OnChannelError();	
  }	
}	
	
}  // namespace IPC