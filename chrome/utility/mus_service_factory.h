// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_MUS_SERVICE_FACTORY_H_
#define CHROME_UTILITY_MUS_SERVICE_FACTORY_H_

#include "build/build_config.h"
#include "content/public/utility/content_utility_client.h"

// Registers services for the mojo ui service.
void RegisterMusServices(
    content::ContentUtilityClient::StaticServiceMap* services);

#if defined(OS_CHROMEOS)
// Registers additional services for --mash.
void RegisterMashServices(
    content::ContentUtilityClient::StaticServiceMap* services);
#endif

#endif  // CHROME_UTILITY_MUS_SERVICE_FACTORY_H_
