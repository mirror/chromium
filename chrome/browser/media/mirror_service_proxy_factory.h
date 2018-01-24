// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_FACTORY_H_
#define CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_FACTORY_H_

#include "base/gtest_prod_util.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace media {

class MirrorServiceProxy;

// A factory that lazily returns a MirrorService implementation for a given
// BrowserContext.
class MirrorServiceProxyFactory : public BrowserContextKeyedServiceFactory {
 public:
  static MirrorServiceProxyFactory* GetInstance();
  static MirrorServiceProxy* GetApiForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::LazyInstanceTraitsBase<MirrorServiceProxyFactory>;

  MirrorServiceProxyFactory();
  ~MirrorServiceProxyFactory() override;

  // BrowserContextKeyedServiceFactory interface.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceProxyFactory);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_FACTORY_H_
