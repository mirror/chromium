// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_RESOLVE_TIME_ZONE_BY_GEOLOCATION_METHOD_SHORT_H_
#define CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_RESOLVE_TIME_ZONE_BY_GEOLOCATION_METHOD_SHORT_H_

#include <memory>

class Profile;

namespace extensions {

class GeneratedPrefImplBase;

extern const char kResolveTimezoneByGeolocationMetodShort[];

// Consrtuctor for kResolveTimezoneByGeolocationMetodShort preference.
std::unique_ptr<GeneratedPrefImplBase>
CreateGeneratedResolveTimezoneByGeolocationMetodShort(Profile* profile);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_CHROMEOS_RESOLVE_TIME_ZONE_BY_GEOLOCATION_METHOD_SHORT_H_
