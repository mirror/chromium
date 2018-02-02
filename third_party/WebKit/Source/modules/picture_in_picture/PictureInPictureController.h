// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureController_h
#define PictureInPictureController_h

#include "core/frame/LocalFrame.h"
#include "modules/ModulesExport.h"

namespace blink {

class HTMLVideoElement;

class MODULES_EXPORT PictureInPictureController
    : public GarbageCollectedFinalized<PictureInPictureController>,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(PictureInPictureController);
  WTF_MAKE_NONCOPYABLE(PictureInPictureController);

 public:
  virtual ~PictureInPictureController();

  static PictureInPictureController& Ensure(Document&);

  static const char* SupplementName();

  // Returns whether Picture-in-Picture is enabled or not.
  bool PictureInPictureEnabled() const;

  // Used only in tests to enable or disable Picture-in-Picture.
  void SetPictureInPictureEnabledForTesting(bool);

  enum class Status {
    kEnabled,
    kDisabledBySystem,
    kDisabledByFeaturePolicy,
    kDisabledByAttribute,
  };

  // Returns Picture-in-Picture status for a document.
  Status IsDocumentAllowed() const;

  // Returns Picture-in-Picture status for a given element in a document.
  Status IsElementAllowed(HTMLVideoElement&) const;

  // Set Picture-in-Picture element.
  void SetPictureInPictureElement(HTMLVideoElement&);

  // Set Picture-in-Picture element to null.
  void UnsetPictureInPictureElement();

  // Returns the Picture-in-Picture element.
  HTMLVideoElement* PictureInPictureElement() const;

  // Implementation of Supplement.
  void Trace(blink::Visitor*) override;

 private:
  explicit PictureInPictureController(Document&);

  // Whether Picture-in-Picture is enabled or not.
  bool picture_in_picture_enabled_ = true;

  // The Picture-in-Picture element for a document.
  Member<HTMLVideoElement> picture_in_picture_element_;
};

}  // namespace blink

#endif  // PictureInPictureController_h
