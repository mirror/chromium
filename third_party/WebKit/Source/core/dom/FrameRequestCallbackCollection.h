// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FrameRequestCallbackCollection_h
#define FrameRequestCallbackCollection_h

#include "bindings/core/v8/v8_frame_request_callback.h"
#include "core/CoreExport.h"
#include "core/dom/DOMHighResTimeStamp.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/heap/Handle.h"

namespace blink {

class ExecutionContext;

class CORE_EXPORT FrameRequestCallbackCollection final
    : public TraceWrapperBase {
  DISALLOW_NEW();

 public:
  explicit FrameRequestCallbackCollection(ExecutionContext*);

  using CallbackId = int;

  // |FrameCallback| is an interface type which generalizes callbacks which are
  // invoked when a script-based animation needs to be resampled.
  class CORE_EXPORT FrameCallback
      : public GarbageCollectedFinalized<FrameCallback>,
        public TraceWrapperBase {
   public:
    DEFINE_INLINE_VIRTUAL_TRACE() {}
    DEFINE_INLINE_VIRTUAL_TRACE_WRAPPERS() {}
    virtual ~FrameCallback() = default;
    virtual void invoke(double) = 0;

    int GetId() const { return id_; }
    bool GetCancelled() const { return cancelled_; }
    bool GetUse_legacy_time_base() const { return use_legacy_time_base_; }
    void SetId(int id) { id_ = id; }
    void SetCancelled(bool cancelled) { cancelled_ = cancelled; }
    void SetUse_legacy_time_base(bool use_legacy_time_base) {
      use_legacy_time_base_ = use_legacy_time_base;
    }

   private:
    int id_ = 0;
    bool cancelled_ = false;
    bool use_legacy_time_base_ = false;
  };

  // |V8FrameCallback| is an adapter class for the conversion from
  // |V8FrameRequestCallback| to |Framecallback|.
  class CORE_EXPORT V8FrameCallback : public FrameCallback {
   public:
    static V8FrameCallback* Create(V8FrameRequestCallback* callback) {
      return new V8FrameCallback(callback);
    }
    ~V8FrameCallback() = default;
    void invoke(double) override;
    DECLARE_TRACE();
    DECLARE_TRACE_WRAPPERS();

   private:
    explicit V8FrameCallback(V8FrameRequestCallback*);
    TraceWrapperMember<V8FrameRequestCallback> callback_;
  };

  CallbackId RegisterCallback(FrameCallback*);
  void CancelCallback(CallbackId);
  void ExecuteCallbacks(double high_res_now_ms, double high_res_now_ms_legacy);

  bool IsEmpty() const { return !callbacks_.size(); }

  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();

 private:
  using CallbackList = HeapVector<TraceWrapperMember<FrameCallback>>;
  CallbackList callbacks_;
  CallbackList
      callbacks_to_invoke_;  // only non-empty while inside executeCallbacks

  CallbackId next_callback_id_ = 0;

  Member<ExecutionContext> context_;
};

}  // namespace blink

#endif  // FrameRequestCallbackCollection_h
