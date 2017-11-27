// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyed_service/core/keyed_service.h"

KeyedService::KeyedService() = default;

KeyedService::~KeyedService() = default;

void KeyedService::Shutdown() {}
