// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebPresentationReceiver_h
#define WebPresentationReceiver_h

#include "public/platform/WebCommon.h"

namespace blink {

// The delegate Blink provides to WebPresentationReceiverClient in order to get
// updates.
class BLINK_PLATFORM_EXPORT WebPresentationReceiver {
 public:
  virtual ~WebPresentationReceiver() = default;

  // Initializes the PresentationReceiver object, such as setting up Mojo
  // connections. No-ops if already initialized.
  virtual void InitIfNeeded() = 0;

  virtual void OnReceiverTerminated() = 0;
};

}  // namespace blink

#endif  // WebPresentationReceiver_h
