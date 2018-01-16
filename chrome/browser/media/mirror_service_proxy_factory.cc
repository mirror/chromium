// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_proxy_factory.h"

#include "build/build_config.h"
#include "chrome/browser/media/mirror_service_proxy.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

using content::BrowserContext;

namespace media {

namespace {

base::LazyInstance<MirrorServiceProxyFactory>::DestructorAtExit
    mirror_service_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

MirrorServiceProxyFactory::MirrorServiceProxyFactory()
    : BrowserContextKeyedServiceFactory(
          "MirrorService",
          BrowserContextDependencyManager::GetInstance()) {}

MirrorServiceProxyFactory::~MirrorServiceProxyFactory() {}

// static
MirrorServiceProxyFactory* MirrorServiceProxyFactory::GetInstance() {
  DVLOG(3) << __func__;
  return &mirror_service_factory.Get();
}

// static
MirrorServiceProxy* MirrorServiceProxyFactory::GetApiForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  DVLOG(3) << __func__;
  return static_cast<MirrorServiceProxy*>(
      mirror_service_factory.Get().GetServiceForBrowserContext(context, true));
}

content::BrowserContext* MirrorServiceProxyFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  DVLOG(3) << __func__;
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* MirrorServiceProxyFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  DVLOG(3) << __func__;
  MirrorServiceProxy* mirror_service_proxy = new MirrorServiceProxy();
  return mirror_service_proxy;
}

}  // namespace media
