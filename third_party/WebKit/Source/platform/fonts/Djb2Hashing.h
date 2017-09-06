// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Djb2Hashing_h
#define Djb2Hashing_h

namespace blink {

inline void AddToHash(unsigned& hash, unsigned key) {
  hash = ((hash << 5) + hash) + key;  // Djb2
};

inline void AddFloatToHash(unsigned& hash, float value) {
  AddToHash(hash, WTF::FloatHash<float>::GetHash(value));
};

}  // namespace blink

#endif  // Djb2Hashing_h
