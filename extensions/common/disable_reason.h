// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_DISABLE_REASON_H_
#define EXTENSIONS_COMMON_DISABLE_REASON_H_

namespace extensions {

// Reasons a Chrome extension may be disabled. These are used in histograms, so
// do not remove/reorder entries - only add at the end just before
// DISABLE_REASON_LAST (and update the shift value for it). Also remember to
// update the enum listing in tools/metrics/histograms.xml.
// Also carefully consider if your reason should sync to other devices, and if
// so, add it to kKnownSyncableDisableReasons in
// chrome/browser/extensions/extension_sync_service.cc.
enum ExtensionDisableReason {
  EXTENSION_DISABLE_NONE = 0,
  EXTENSION_DISABLE_USER_ACTION = 1 << 0,
  EXTENSION_DISABLE_PERMISSIONS_INCREASE = 1 << 1,
  EXTENSION_DISABLE_RELOAD = 1 << 2,
  EXTENSION_DISABLE_UNSUPPORTED_REQUIREMENT = 1 << 3,
  EXTENSION_DISABLE_SIDELOAD_WIPEOUT = 1 << 4,
  EXTENSION_DEPRECATED_DISABLE_UNKNOWN_FROM_SYNC = 1 << 5,
  // EXTENSION_DISABLE_PERMISSIONS_CONSENT = 1 << 6,  // Deprecated.
  // EXTENSION_DISABLE_KNOWN_DISABLED = 1 << 7,  // Deprecated.
  EXTENSION_DISABLE_NOT_VERIFIED = 1 << 8,  // Disabled because we could not
                                            // verify the install.
  EXTENSION_DISABLE_GREYLIST = 1 << 9,
  EXTENSION_DISABLE_CORRUPTED = 1 << 10,
  EXTENSION_DISABLE_REMOTE_INSTALL = 1 << 11,
  // EXTENSION_DISABLE_INACTIVE_EPHEMERAL_APP = 1 << 12,  // Deprecated.
  EXTENSION_DISABLE_EXTERNAL_EXTENSION = 1 << 13,  // External extensions might
                                                   // be disabled for user
                                                   // prompting.
  EXTENSION_DISABLE_UPDATE_REQUIRED_BY_POLICY = 1 << 14,    // Doesn't meet
                                                            // minimum version
                                                            // requirement.
  EXTENSION_DISABLE_CUSTODIAN_APPROVAL_REQUIRED = 1 << 15,  // Supervised user
                                                            // needs approval by
                                                            // custodian.
  EXTENSION_DISABLE_REASON_LAST =
      1 << 16,  // This should always be the last value
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_DISABLE_REASON_H_
