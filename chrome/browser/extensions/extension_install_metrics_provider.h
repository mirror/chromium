// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_METRICS_PROVIDER_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_METRICS_PROVIDER_H_

#include <vector>

#include "base/macros.h"
#include "components/metrics/metrics_provider.h"
#include "components/metrics/proto/extension_install.pb.h"

class Profile;

namespace extensions {
class Extension;
class ExtensionPrefs;

class ExtensionInstallMetricsProvider : public metrics::MetricsProvider {
 public:
  ExtensionInstallMetricsProvider();
  ~ExtensionInstallMetricsProvider() override;

  // metrics::MetricsProvider:
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;

 private:
  friend class ExtensionInstallMetricsProviderTest;

  // Create the install proto for a given |extension|.
  metrics::ExtensionInstallProto ConstructProto(const Extension& extension,
                                                ExtensionPrefs* prefs);

  // Return all the extension installs for a given |profile|.
  std::vector<metrics::ExtensionInstallProto> GetInstallsForProfile(
      Profile* profile);

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallMetricsProvider);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_METRICS_PROVIDER_H_
