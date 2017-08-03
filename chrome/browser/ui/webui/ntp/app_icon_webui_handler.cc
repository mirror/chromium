// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/app_icon_webui_handler.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_icon_manager.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/history/core/browser/top_sites.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_registry.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/color_analysis.h"

namespace {

base::Value* SkColorToCss(SkColor color) {
  return new base::Value(
      base::StringPrintf("rgb(%d, %d, %d)", SkColorGetR(color),
                         SkColorGetG(color), SkColorGetB(color)));
}

base::Value* GetDominantColorCssString(
    scoped_refptr<base::RefCountedMemory> png) {
  color_utils::GridSampler sampler;
  SkColor color = color_utils::CalculateKMeanColorOfPNG(png);
  return SkColorToCss(color);
}

}  // namespace

// Thin inheritance-dependent trampoline to forward notification of app
// icon loads to the AppIconWebUIHandler. Base class does caching of icons.
class AppIconColorManager : public ExtensionIconManager {
 public:
  explicit AppIconColorManager(AppIconWebUIHandler* handler)
      : ExtensionIconManager(), handler_(handler) {}
  ~AppIconColorManager() override {}

  void OnImageLoaded(const std::string& extension_id,
                     const gfx::Image& image) override {
    ExtensionIconManager::OnImageLoaded(extension_id, image);
    handler_->NotifyAppIconReady(extension_id);
  }

 private:
  AppIconWebUIHandler* handler_;
};

AppIconWebUIHandler::AppIconWebUIHandler()
    : app_icon_color_manager_(new AppIconColorManager(this)) {}

AppIconWebUIHandler::~AppIconWebUIHandler() {}

void AppIconWebUIHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAppIconDominantColor",
      base::Bind(&AppIconWebUIHandler::HandleGetAppIconDominantColor,
                 base::Unretained(this)));
}

void AppIconWebUIHandler::HandleGetAppIconDominantColor(
    const base::ListValue* args) {
  std::string extension_id;
  CHECK(args->GetString(0, &extension_id));

  Profile* profile = Profile::FromWebUI(web_ui());
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(profile);
  const extensions::Extension* extension =
      extension_registry->enabled_extensions().GetByID(extension_id);
  if (!extension)
    return;
  app_icon_color_manager_->LoadIcon(profile, extension);
}

void AppIconWebUIHandler::NotifyAppIconReady(const std::string& extension_id) {
  gfx::Image icon = app_icon_color_manager_->GetIcon(extension_id);
  // TODO(estade): would be nice to avoid a round trip through png encoding.
  std::vector<unsigned char> bits;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(*icon.ToSkBitmap(), true, &bits))
    return;
  scoped_refptr<base::RefCountedStaticMemory> bits_mem(
      new base::RefCountedStaticMemory(&bits.front(), bits.size()));
  std::unique_ptr<base::Value> color_value(GetDominantColorCssString(bits_mem));
  base::Value id(extension_id);
  web_ui()->CallJavascriptFunctionUnsafe("ntp.setFaviconDominantColor", id,
                                         *color_value);
}
