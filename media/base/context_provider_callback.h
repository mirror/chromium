// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_CONTEXT_PROVIDER_CALLBACK_H_
#define MEDIA_BASE_CONTEXT_PROVIDER_CALLBACK_H_

#include "base/callback.h"

namespace viz {
class ContextProvider;
}

namespace media {

// Callback to obtain the media ContextProvider.
// Requires being called on the media thread.
// The argument callback is also called on the media thread as a reply.
typedef base::Callback<void(base::Callback<void(viz::ContextProvider*)>)>
    ContextProviderCallback;

}  // namespace media

#endif  // MEDIA_BASE_CONTEXT_PROVIDER_CALLBACK_H_
