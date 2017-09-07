// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FRAME_MESSAGES_FORWARD_H_
#define CONTENT_COMMON_FRAME_MESSAGES_FORWARD_H_

// Forward-declaration for FrameHostMsg_DidCommitProvisionalLoad_Params, which
// is typemapped to a Mojo [Native] type used by content::mojom::FrameHost.
// This requires the generated header for mojom::FrameHost a forward declaration
// of this legacy IPC struct type.
//
// However, legacy IPC struct type definition headers must be included exactly
// once in each translation unit using them, so .cc files directly or indirectly
// including content/common/frame_messages.h cannot include the Mojo headers
// anymore. Hence the generated header for mojom::FrameHost does not include
// frame_messages.h, but instead only include this forward-declaration.
//
// TODO(https://crbug.com/729021): Eventually convert this legacy IPC struct to
// a proper Mojo type.
struct FrameHostMsg_DidCommitProvisionalLoad_Params;

#endif  // CONTENT_COMMON_FRAME_MESSAGES_FORWARD_H_
