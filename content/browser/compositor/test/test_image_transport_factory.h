// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_

#include <memory>

#include "content/browser/compositor/image_transport_factory.h"

namespace content {

// Creates an appropriate ImageTransportFactory implementation for tests,
// which changes depending on if the --enable-viz flag is present.
std::unique_ptr<ImageTransportFactory> CreateImageTransportFactoryForTesting();

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_TEST_TEST_IMAGE_TRANSPORT_FACTORY_H_
