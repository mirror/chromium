// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebScreenAvailability_h
#define WebScreenAvailability_h

namespace blink {

// Various states for the remote playback device or presentation screen
// availability.
enum class WebScreenAvailability {
  // The availability is unknown.
  kUnknown,

  // Availability monitoring is not supported.
  kUnsupported,

  // No screens are available.
  kUnavailable,

  // The source specified for availability monitoring is not compatible with
  // supported screens. Screens may or may not be available (only used for
  // RemotePlayback API / Media Flinging).
  kSourceNotCompatible,

  // There're available screens and the source is compatible with at least one
  // of them.
  kAvailable,

  kLast = kAvailable
};

}  // namespace blink

#endif  // WebScreenAvailability_h
