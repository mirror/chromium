// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is defined here to ensure the D3D11, D3D9, and DXVA includes through
// this header have their GUIDs intialized.
#define INITGUID
#include "media/gpu/d3d11_video_decode_accelerator_win.h"
#undef INITGUID
