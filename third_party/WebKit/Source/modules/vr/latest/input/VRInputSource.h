// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRInputSource_h
#define VRInputSource_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRSession;

enum VRInputSourceType {
  kVRPointerInputSource,
  kVRGamepadInputSource,
  kVRControllerInputSource,
};

class VRInputSource : public GarbageCollectedFinalized<VRInputSource>,
                      public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRInputSource(VRSession*,
                uint32_t index,
                VRInputSourceType,
                bool gaze_cursor);
  virtual ~VRInputSource();

  VRSession* session() const { return session_; }
  VRInputSourceType inputType() const { return input_type_; }
  uint32_t index() const { return index_; }

  bool gaze_cursor() const { return gaze_cursor_; }

  void set_base_pose_matrix(std::unique_ptr<TransformationMatrix>);
  void set_pointer_transform_matrix(std::unique_ptr<TransformationMatrix>);

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class VRPresentationFrame;

  const Member<VRSession> session_;
  const uint32_t index_;
  const VRInputSourceType input_type_;
  const bool gaze_cursor_;

  std::unique_ptr<TransformationMatrix> base_pose_matrix_;

  // This is the transform to apply to the base_pose_matrix_ to get the pointer
  // matrix. In most cases it should be static.
  std::unique_ptr<TransformationMatrix> pointer_transform_matrix_;
};

}  // namespace blink

#endif  // VRLayer_h
