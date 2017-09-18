// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ActivationDelegationFlags_h
#define ActivationDelegationFlags_h

namespace blink {

// To add a new activation delegation context, add a flag here and then add
// it to core/html/HTMLIFrameElementAllowedActivationDelegation.cpp.
enum ActivationDelegationFlag {
  kActivationDelegationNone = 0,

  // Allow user activation delegation to a child frame in the context of media
  // playback (e.g. autoplay).
  kActivationDelegationMedia = 1,
};

typedef int ActivationDelegationFlags;

}  // namespace blink

#endif
