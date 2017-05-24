// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRView_h
#define VRView_h

#include "core/dom/DOMTypedArray.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class VRLayer;
class VRViewport;

class VRView final : public GarbageCollectedFinalized<VRView>,
                     public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRView(const String& eye, DOMFloat32Array* projection_matrix);

  const String& eye() const { return eye_; }
  DOMFloat32Array* projectionMatrix() const { return projection_matrix_; }
  VRViewport* getViewport(VRLayer*) const;

  DECLARE_VIRTUAL_TRACE();

 private:
  String eye_;
  Member<DOMFloat32Array> projection_matrix_;
};

}  // namespace blink

#endif  // VRView_h
