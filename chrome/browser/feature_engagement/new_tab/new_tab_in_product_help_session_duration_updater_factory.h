// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_

#include "base/macros.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_in_product_help_session_duration_updater.h"
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

class NewTabInProductHelpSessionDurationUpdater;

// NewTabInProductHelpSessionDurationUpdaterFactory is the main client class for
// interaction with the NewTabInProductHelpSessionDurationUpdater component.
class NewTabInProductHelpSessionDurationUpdaterFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of
  // NewTabInProductHelpSessionDurationUpdaterFactory.
  static NewTabInProductHelpSessionDurationUpdaterFactory* GetInstance();

  // Returns the NewTabInProductHelpSessionDurationUpdater associated with the
  // profile.
  NewTabInProductHelpSessionDurationUpdater* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      NewTabInProductHelpSessionDurationUpdaterFactory>;

  NewTabInProductHelpSessionDurationUpdaterFactory();
  ~NewTabInProductHelpSessionDurationUpdaterFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(NewTabInProductHelpSessionDurationUpdaterFactory);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_IN_PRODUCT_HELP_SESSION_DURATION_UPDATER_FACTORY_H_
