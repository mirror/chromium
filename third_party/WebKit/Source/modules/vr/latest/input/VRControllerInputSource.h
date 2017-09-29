// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRControllerInputSource_h
#define VRControllerInputSource_h

#include "modules/vr/latest/input/VRControllerElementMap.h"
#include "modules/vr/latest/input/VRInputSource.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class VRSession;

class VRControllerInputSource : public VRInputSource {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum Handedness { kHandNone, kHandLeft, kHandRight };

  VRControllerInputSource(
      VRSession*,
      uint32_t index,
      bool gaze_cursor,
      const HeapHashMap<String, Member<VRControllerElement>>&);
  ~VRControllerInputSource() override;

  const String& handedness() const { return handedness_string_; }
  void setHandedness(Handedness);

  VRControllerElementMap* elements() const { return elements_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  Handedness handedness_ = kHandNone;
  String handedness_string_;
  Member<VRControllerElementMap> elements_;
};

}  // namespace blink

#endif  // VRWebGLLayer_h
