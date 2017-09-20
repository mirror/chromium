// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InternalsAnimationWorklet_h
#define InternalsAnimationWorklet_h

#include "platform/wtf/text/WTFString.h"

namespace blink {

class Internals;
class ScriptState;

class InternalsAnimationWorklet {
  STATIC_ONLY(InternalsAnimationWorklet);

 public:
  // Instantiates a new animator instance for testing.
  // TODO(majidvp): Remove this once we have and an alternative way to
  // instantiate.
  static void createAnimatorForTest(ScriptState*,
                                    Internals&,
                                    int player_id,
                                    const String& name);
};

}  // namespace blink

#endif  // InternalsAnimationWorklet_h
