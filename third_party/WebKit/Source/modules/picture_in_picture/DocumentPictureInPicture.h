// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentPictureInPicture_h
#define DocumentPictureInPicture_h

#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class Document;
class ScriptPromise;
class ScriptState;

class DocumentPictureInPicture final
    : public GarbageCollected<DocumentPictureInPicture> {
 public:
  static DocumentPictureInPicture* Create() {
    return new DocumentPictureInPicture();
  }

  static bool pictureInPictureEnabled(const Document&);
  static ScriptPromise exitPictureInPicture(ScriptState*, const Document&);

  void Trace(blink::Visitor*) {}

 private:
  DocumentPictureInPicture() = default;
};

}  // namespace blink

#endif  // DocumentPictureInPicture_h
