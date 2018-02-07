// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureController_h
#define PictureInPictureController_h

#include "core/frame/LocalFrame.h"
#include "modules/ModulesExport.h"

namespace blink {

class HTMLVideoElement;
class PictureInPictureWindow;

class MODULES_EXPORT PictureInPictureController
    : public GarbageCollectedFinalized<PictureInPictureController>,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(PictureInPictureController);
  WTF_MAKE_NONCOPYABLE(PictureInPictureController);

 public:
  virtual ~PictureInPictureController();

  static PictureInPictureController& Ensure(Document&);

  static const char* SupplementName();

  bool PictureInPictureEnabled() const;

  void SetPictureInPictureEnabledForTesting(bool);

  enum class Status {
    kEnabled,
    kFrameDetached,
    kDisabledBySystem,
    kDisabledByFeaturePolicy,
    kDisabledByAttribute,
  };

  Status IsDocumentAllowed() const;

  Status IsElementAllowed(HTMLVideoElement&) const;

  void SetPictureInPictureElement(HTMLVideoElement&);

  void UnsetPictureInPictureElement();

  HTMLVideoElement* PictureInPictureElement() const;

  // Meant to be called by HTMLVideoElementPictureInPicture, and DOM objects but
  // not internally.
  PictureInPictureWindow* CreatePictureInPictureWindow(int, int);

  // Meant to be called by DocumentPictureInPicture,
  // HTMLVideoElementPictureInPicture, and DOM objects but not internally.
  void SetNullDimensionsForPictureInPictureWindow();

  void Trace(blink::Visitor*) override;

 private:
  explicit PictureInPictureController(Document&);

  bool picture_in_picture_enabled_ = true;

  Member<HTMLVideoElement> picture_in_picture_element_;

  // The Picture-in-Picture window for the associated document.
  Member<PictureInPictureWindow> picture_in_picture_window_;
};

}  // namespace blink

#endif  // PictureInPictureController_h
