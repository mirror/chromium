// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PreemptionManager_h
#define PreemptionManager_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/wtf/Time.h"
#include "platform/wtf/WeakPtr.h"

namespace v8 {
class Isolate;
}

namespace blink {

class CORE_EXPORT PreemptionManager : public V8PerIsolateData::Data {
 public:
  explicit PreemptionManager(v8::Isolate*);
  ~PreemptionManager();

  bool InPreemptionCheckpointScope() const;
  void EnterPreemptionCheckpointScope();
  void LeavePreemptionCheckpointScope();

  bool InPreemptionOptInScope() const;
  void EnterPreemptionOptInScope();
  void LeavePreemptionOptInScope();

  WTF::WeakPtr<PreemptionManager> GetWeakPtr();

 private:
  void RunNestedLoop();

  v8::Isolate* isolate_ = nullptr;

  bool in_preemption_checkpoint_ = false;
  bool in_preemption_opt_in_ = false;
  WTF::TimeTicks next_;

  WeakPtrFactory<PreemptionManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreemptionManager);
};

}  // namespace blink

#endif  // PreemptionManager_h
