// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GestureDelegationFlags_h
#define GestureDelegationFlags_h

namespace blink {

// To add a new gesture delegation context, add a flag here and then add
// it to core/html/HTMLIFrameElementAllowedGestureDelegation.cpp.
enum GestureDelegationFlag {
  kGestureDelegationNone = 0,

  // Allow gesture delegation to a child frame in the context of media
  // playback (e.g. autoplay).
  kGestureDelegationMedia = 1,
};

typedef int GestureDelegationFlags;

}  // namespace blink

#endif
