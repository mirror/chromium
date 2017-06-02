// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/SVGRendererMailboxReceiver.h"

#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

SVGRendererMailboxReceiver::SVGRendererMailboxReceiver() {
  DCHECK(!service_.is_bound());
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&service_));
}

}  // namespace blink
