// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/audio/cpu/x86/VectorMathAVX.h"

#if defined(__AVX__)
#include "platform/audio/cpu/x86/VectorMathImpl.cpp"
#endif

// Check that VectorMathImpl.cpp defined the blink::VectorMath::AVX namespace.
static_assert(sizeof(blink::VectorMath::AVX::MType), "");
