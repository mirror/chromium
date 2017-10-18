// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/FontFaceSet.h"

namespace blink {

void FontFaceSet::LoadFontPromiseResolver::LoadFonts() {
  if (!num_loading_) {
    resolver_->Resolve(font_faces_);
    return;
  }

  for (size_t i = 0; i < font_faces_.size(); i++)
    font_faces_[i]->LoadWithCallback(this);
}

void FontFaceSet::LoadFontPromiseResolver::NotifyLoaded(FontFace* font_face) {
  num_loading_--;
  if (num_loading_ || error_occured_)
    return;

  resolver_->Resolve(font_faces_);
}

void FontFaceSet::LoadFontPromiseResolver::NotifyError(FontFace* font_face) {
  num_loading_--;
  if (!error_occured_) {
    error_occured_ = true;
    resolver_->Reject(font_face->GetError());
  }
}

DEFINE_TRACE(FontFaceSet::LoadFontPromiseResolver) {
  visitor->Trace(font_faces_);
  visitor->Trace(resolver_);
  LoadFontCallback::Trace(visitor);
}

bool FontFaceSet::IterationSource::Next(ScriptState*,
                                        Member<FontFace>& key,
                                        Member<FontFace>& value,
                                        ExceptionState&) {
  if (font_faces_.size() <= index_)
    return false;
  key = value = font_faces_[index_++];
  return true;
}

}  // namespace blink
