// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_

#include "base/macros.h"
#include "chrome/browser/feature_engagement/incognito_window/incognito_window_in_product_help_session_duration_updater.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace feature_engagement {

class IncognitoWindowInProductHelpSessionDurationUpdater;

// IncognitoWindowInProductHelpSessionDurationUpdaterFactory is the main client
// class for interaction with the
// IncognitoWindowInProductHelpSessionDurationUpdater component.
class IncognitoWindowInProductHelpSessionDurationUpdaterFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of
  // IncognitoWindowInProductHelpSessionDurationUpdaterFactory.
  static IncognitoWindowInProductHelpSessionDurationUpdaterFactory*
  GetInstance();

  // Returns the IncognitoWindowInProductHelpSessionDurationUpdater associated
  // with the profile.
  IncognitoWindowInProductHelpSessionDurationUpdater* GetForProfile(
      Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      IncognitoWindowInProductHelpSessionDurationUpdaterFactory>;

  IncognitoWindowInProductHelpSessionDurationUpdaterFactory();
  ~IncognitoWindowInProductHelpSessionDurationUpdaterFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(
      IncognitoWindowInProductHelpSessionDurationUpdaterFactory);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_
