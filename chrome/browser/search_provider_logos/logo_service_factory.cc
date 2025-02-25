// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search_provider_logos/logo_service_factory.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/search_provider_logos/logo_service.h"
#include "components/search_provider_logos/logo_service_impl.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/feature_utilities.h"
#endif

using search_provider_logos::LogoService;
using search_provider_logos::LogoServiceImpl;

namespace {

constexpr base::FilePath::CharType kCachedLogoDirectory[] =
    FILE_PATH_LITERAL("Search Logos");

bool UseGrayLogo() {
#if defined(OS_ANDROID)
  return !chrome::android::GetIsChromeModernDesignEnabled();
#else
  return false;
#endif
}

}  // namespace

// static
LogoService* LogoServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<LogoService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
LogoServiceFactory* LogoServiceFactory::GetInstance() {
  return base::Singleton<LogoServiceFactory>::get();
}

LogoServiceFactory::LogoServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "LogoService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

LogoServiceFactory::~LogoServiceFactory() = default;

KeyedService* LogoServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  DCHECK(!profile->IsOffTheRecord());
  return new LogoServiceImpl(
      profile->GetPath().Append(kCachedLogoDirectory),
      GaiaCookieManagerServiceFactory::GetForProfile(profile),
      TemplateURLServiceFactory::GetForProfile(profile),
      base::MakeUnique<suggestions::ImageDecoderImpl>(),
      profile->GetRequestContext(), base::BindRepeating(&UseGrayLogo));
}
