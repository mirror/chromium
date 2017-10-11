// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_PLATFORM_API_SPDY_TEST_H_
#define NET_SPDY_PLATFORM_API_SPDY_TEST_H_

#include "net/spdy/platform/impl/spdy_test_impl.h"

// Defines the base classes to be used in HTTP/2 tests.
using SpdyTest = SpdyTestImpl;

template <class T>
using SpdyTestWithParam = SpdyTestWithParamImpl<T>;

#endif  // NET_SPDY_PLATFORM_API_SPDY_TEST_H_
