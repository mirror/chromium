// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/mash_service_registry.h"

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "mash/quick_launch/public/interfaces/constants.mojom.h"
#include "mash/quick_launch/quick_launch.h"
#include "services/ui/public/interfaces/constants.mojom.h"

#if defined(OS_CHROMEOS)
#include "ash/public/interfaces/constants.mojom.h"  // nogncheck
#endif                                              // defined(OS_CHROMEOS)

#if defined(OS_LINUX) && !defined(OS_ANDROID)
#include "components/font_service/public/interfaces/constants.mojom.h"
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)

void RegisterOutOfProcessServicesForMash(
    content::ContentBrowserClient::OutOfProcessServiceMap* services) {
  (*services)[mash::quick_launch::mojom::kServiceName] =
      base::ASCIIToUTF16("Quick Launch");
  (*services)[ui::mojom::kServiceName] = base::ASCIIToUTF16("UI Service");
#if defined(OS_CHROMEOS)
  (*services)[ash::mojom::kServiceName] =
      base::ASCIIToUTF16("Ash Window Manager and Shell");
  (*services)["accessibility_autoclick"] =
      base::ASCIIToUTF16("Ash Accessibility Autoclick");
  (*services)["touch_hud"] = base::ASCIIToUTF16("Ash Touch Hud");
#endif  // defined(OS_CHROMEOS)
#if defined(OS_LINUX) && !defined(OS_ANDROID)
  (*services)[font_service::mojom::kServiceName] =
      base::ASCIIToUTF16("Font Service");
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)
}
