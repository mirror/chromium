// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRPresentationFrame_h
#define VRPresentationFrame_h

#include "device/vr/vr_service.mojom-blink.h"
#include "modules/vr/latest/VRView.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRCoordinateSystem;
class VRDevicePose;
class VRSession;

class VRPresentationFrame final
    : public GarbageCollectedFinalized<VRPresentationFrame>,
      public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRPresentationFrame(VRSession*);

  const HeapVector<Member<VRView>>& views() const { return views_; }
  VRDevicePose* getDevicePose(VRCoordinateSystem*) const;

  void UpdateBasePose(std::unique_ptr<TransformationMatrix>);

  VRSession* session() const { return session_; }

  DECLARE_VIRTUAL_TRACE();

  DECLARE_VIRTUAL_TRACE_WRAPPERS();

 private:
  const TraceWrapperMember<VRSession> session_;
  HeapVector<Member<VRView>> views_;
  std::unique_ptr<TransformationMatrix> base_pose_matrix_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
