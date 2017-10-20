// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace machine_intelligence {

class AssistRankerService;

class AssistRankerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static AssistRankerServiceFactory* GetInstance();
  static machine_intelligence::AssistRankerService* GetForBrowserContext(
      content::BrowserContext* browser_context);

 private:
  friend struct base::DefaultSingletonTraits<AssistRankerServiceFactory>;

  AssistRankerServiceFactory();
  ~AssistRankerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AssistRankerServiceFactory);
};

}  // namespace machine_intelligence

#endif  // CHROME_BROWSER_MACHINE_INTELLIGENCE_ASSIST_RANKER_SERVICE_FACTORY_H_
