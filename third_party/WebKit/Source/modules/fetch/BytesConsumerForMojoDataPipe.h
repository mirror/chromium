// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BytesConsumerForMojoDataPipe_h
#define BytesConsumerForMojoDataPipe_h

#include "modules/ModulesExport.h"
#include "modules/fetch/BytesConsumer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/text/WTFString.h"

#include <memory>

namespace blink {

// This is very similar to BytesConsumerForDataConsumerHandle but it uses the
// native Mojo data pipe directly.
class MODULES_EXPORT BytesConsumerForMojoDataPipe final : public BytesConsumer {
  EAGERLY_FINALIZE();
  DECLARE_EAGER_FINALIZATION_OPERATOR_NEW();

 public:
  BytesConsumerForMojoDataPipe(ExecutionContext*,
                               mojo::ScopedDataPipeConsumerHandle);
  ~BytesConsumerForMojoDataPipe() override;

  // BytesConsumer implementation:
  Result BeginRead(const char** buffer, size_t* available) override;
  Result EndRead(size_t read_size) override;
  void SetClient(BytesConsumer::Client*) override;
  void ClearClient() override;
  void Cancel() override;
  PublicState GetPublicState() const override;
  Error GetError() const override {
    DCHECK(state_ == InternalState::kErrored);
    return error_;
  }
  String DebugName() const override { return "BytesConsumerForMojoDataPipe"; }

  // GarbageCollectedFinalized implementation:
  void Trace(blink::Visitor*) override;

 private:
  void OnHandleGotReadable(MojoResult);
  void Close();
  void SetError();
  void Notify();

  Member<ExecutionContext> execution_context_;
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  mojo::SimpleWatcher handle_watcher_;
  Member<BytesConsumer::Client> client_;
  InternalState state_ = InternalState::kWaiting;
  Error error_;
  bool is_in_two_phase_read_ = false;
  bool has_pending_notification_ = false;
};

}  // namespace blink

#endif  // BytesConsumerForMojoDataPipe_h
