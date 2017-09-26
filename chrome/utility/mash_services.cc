// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/mash_services.h"

#include "base/memory/ptr_util.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

#if defined(OS_CHROMEOS)
#include "ash/mus/window_manager_application.h"     // nogncheck
#include "ash/public/interfaces/constants.mojom.h"  // nogncheck
#endif                                              // defined(OS_CHROMEOS)

#if defined(OS_LINUX) && !defined(OS_ANDROID)
#include "components/font_service/font_service_app.h"
#include "components/font_service/public/interfaces/constants.mojom.h"
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)

namespace {

using FactoryFunction = std::unique_ptr<service_manager::Service>();

void RegisterMashService(
    content::ContentUtilityClient::StaticServiceMap* services,
    const std::string& name,
    FactoryFunction factory_function) {
  service_manager::EmbeddedServiceInfo service_info;
  service_info.factory = base::Bind(factory_function);
  services->emplace(name, service_info);
}

std::unique_ptr<service_manager::Service> CreateUiService() {
  LOG(ERROR) << "UI";
  return base::MakeUnique<ui::Service>();
}

#if defined(OS_CHROMEOS)
std::unique_ptr<service_manager::Service> CreateAshService() {
  LOG(ERROR) << "ASH!";
  const bool show_primary_host_on_connect = true;
  return base::MakeUnique<ash::mus::WindowManagerApplication>(
      show_primary_host_on_connect);
}
#endif

#if defined(OS_LINUX) && !defined(OS_ANDROID)
std::unique_ptr<service_manager::Service> CreateFontService() {
  LOG(ERROR) << "FontService";
  return base::MakeUnique<font_service::FontServiceApp>();
}

#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)

}  // namespace

void RegisterMashServices(
    content::ContentUtilityClient::StaticServiceMap* services) {
  RegisterMashService(services, ui::mojom::kServiceName, &CreateUiService);
  RegisterMashService(services, ash::mojom::kServiceName, &CreateAshService);
/*
  return base::MakeUnique<ui::Service>();
#if defined(OS_CHROMEOS)
if (service_name == ash::mojom::kServiceName) {
  return base::WrapUnique(
      new ash::mus::WindowManagerApplication(show_primary_host_on_connect));
}
if (service_name == "accessibility_autoclick")
  return base::MakeUnique<ash::autoclick::AutoclickApplication>();
if (service_name == "touch_hud")
  return base::MakeUnique<ash::touch_hud::TouchHudApplication>();
#endif  // defined(OS_CHROMEOS)
if (service_name == mash::catalog_viewer::mojom::kServiceName)
  return base::MakeUnique<mash::catalog_viewer::CatalogViewer>();
if (service_name == mash::session::mojom::kServiceName)
  return base::MakeUnique<mash::session::Session>();
if (service_name == mash::quick_launch::mojom::kServiceName)
  return base::MakeUnique<mash::quick_launch::QuickLaunch>();
if (service_name == mash::task_viewer::mojom::kServiceName)
  return base::MakeUnique<mash::task_viewer::TaskViewer>();
if (service_name == "test_ime_driver")
  return base::MakeUnique<ui::test::TestIMEApplication>();
*/
#if defined(OS_LINUX) && !defined(OS_ANDROID)
  RegisterMashService(services, font_service::mojom::kServiceName,
                      &CreateFontService);
#endif  // defined(OS_LINUX) && !defined(OS_ANDROID)
}
