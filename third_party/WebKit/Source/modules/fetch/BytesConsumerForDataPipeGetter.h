// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BytesConsumerForDataPipeGetter_h
#define BytesConsumerForDataPipeGetter_h

#include <memory>

#include "modules/ModulesExport.h"
#include "modules/fetch/BytesConsumer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"
#include "services/network/public/interfaces/data_pipe_getter.mojom-blink.h"

namespace blink {

// This is similar to BytesConsumerForDataConsumerHandle but it uses the
// DataPipeGetter interface.
class MODULES_EXPORT BytesConsumerForDataPipeGetter final
    : public BytesConsumer {
  EAGERLY_FINALIZE();
  DECLARE_EAGER_FINALIZATION_OPERATOR_NEW();

 public:
  BytesConsumerForDataPipeGetter(ExecutionContext*,
                                 network::mojom::blink::DataPipeGetterPtr);
  ~BytesConsumerForDataPipeGetter() override = default;

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
  String DebugName() const override { return "BytesConsumerForDataPipeGetter"; }

  // GarbageCollectedFinalized implementation:
  void Trace(blink::Visitor*) override;

 private:
  void DataPipeGetterCallback(int32_t status, uint64_t size);
  void OnHandleGotReadable(MojoResult);
  void Close();
  void SetError();
  void Notify();

  Member<ExecutionContext> execution_context_;
  network::mojom::blink::DataPipeGetterPtr data_pipe_getter_;
  mojo::ScopedDataPipeConsumerHandle consumer_handle_;
  mojo::SimpleWatcher handle_watcher_;
  Member<BytesConsumer::Client> client_;
  InternalState state_ = InternalState::kWaiting;
  Error error_;
  bool is_in_two_phase_read_ = false;
  bool has_pending_notification_ = false;
  bool was_data_pipe_getter_callback_called_ = false;
};

}  // namespace blink

#endif  // BytesConsumerForDataPipeGetter_h
