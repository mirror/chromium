// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included header, hence no include guard.

#include "extensions/common/features/feature_session_type.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/param_traits_macros.h"

IPC_ENUM_TRAITS_MAX_VALUE(extensions::FeatureSessionType,
                          extensions::FeatureSessionType::LAST)
