// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/mus_service_factory.h"

#include <memory>

#include "base/bind.h"
#include "build/build_config.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

#if defined(OS_CHROMEOS)
#include "ash/autoclick/mus/autoclick_application.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/touch_hud/mus/touch_hud_application.h"
#include "ash/window_manager_service.h"
#include "mash/quick_launch/public/interfaces/constants.mojom.h"
#include "mash/quick_launch/quick_launch.h"
#endif  // defined(OS_CHROMEOS)

#if defined(OS_LINUX) && !defined(OS_ANDROID)
#include "components/font_service/font_service_app.h"
#include "components/font_service/public/interfaces/constants.mojom.h"
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)

namespace {

using ServiceFactoryFunction = std::unique_ptr<service_manager::Service>();

void RegisterService(content::ContentUtilityClient::StaticServiceMap* services,
                     const std::string& name,
                     ServiceFactoryFunction factory_function) {
  service_manager::EmbeddedServiceInfo service_info;
  service_info.factory = base::BindRepeating(factory_function);
  services->emplace(name, service_info);
}

std::unique_ptr<service_manager::Service> CreateUiService() {
  return std::make_unique<ui::Service>();
}

#if defined(OS_CHROMEOS)
std::unique_ptr<service_manager::Service> CreateAshService() {
  const bool show_primary_host_on_connect = true;
  return std::make_unique<ash::WindowManagerService>(
      show_primary_host_on_connect);
}

std::unique_ptr<service_manager::Service> CreateAccessibilityAutoclick() {
  return std::make_unique<ash::autoclick::AutoclickApplication>();
}

std::unique_ptr<service_manager::Service> CreateQuickLaunch() {
  return std::make_unique<mash::quick_launch::QuickLaunch>();
}

std::unique_ptr<service_manager::Service> CreateTouchHud() {
  return std::make_unique<ash::touch_hud::TouchHudApplication>();
}
#endif

#if defined(OS_LINUX) && !defined(OS_ANDROID)
std::unique_ptr<service_manager::Service> CreateFontService() {
  return std::make_unique<font_service::FontServiceApp>();
}
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)

}  // namespace

void RegisterMusServices(
    content::ContentUtilityClient::StaticServiceMap* services) {
  RegisterService(services, ui::mojom::kServiceName, &CreateUiService);
#if defined(OS_LINUX) && !defined(OS_ANDROID)
  RegisterService(services, font_service::mojom::kServiceName,
                  &CreateFontService);
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)
}

#if defined(OS_CHROMEOS)
void RegisterMashServices(
    content::ContentUtilityClient::StaticServiceMap* services) {
  RegisterService(services, mash::quick_launch::mojom::kServiceName,
                  &CreateQuickLaunch);
  RegisterService(services, ash::mojom::kServiceName, &CreateAshService);
  RegisterService(services, "accessibility_autoclick",
                  &CreateAccessibilityAutoclick);
  RegisterService(services, "touch_hud", &CreateTouchHud);
}
#endif  // defined(OS_CHROMEOS)
