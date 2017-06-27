// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media/media_engagement_ui.h"

#include <cmath>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace {

// Implementation of mojom::MediaEngagementDetailsProvider that retrieves
// engagement details from the MediaEngagementService.
class MediaEngagementDetailsProviderImpl
    : public mojom::MediaEngagementDetailsProvider {
 public:
  MediaEngagementDetailsProviderImpl(
      Profile* profile,
      mojo::InterfaceRequest<mojom::MediaEngagementDetailsProvider> request)
      : profile_(profile), binding_(this, std::move(request)) {
    DCHECK(profile_);
  }

  ~MediaEngagementDetailsProviderImpl() override {}

  // mojom::MediaEngagementDetailsProvider overrides:
  void GetMediaEngagementDetails(
      const GetMediaEngagementDetailsCallback& callback) override {
    MediaEngagementService* service = MediaEngagementService::Get(profile_);
    std::vector<mojom::MediaEngagementDetails> details =
        service->GetAllDetails();

    std::vector<mojom::MediaEngagementDetailsPtr> engagement_info;
    engagement_info.reserve(details.size());
    for (const auto& info : details) {
      mojom::MediaEngagementDetailsPtr origin_info(
          mojom::MediaEngagementDetails::New());
      *origin_info = std::move(info);
      engagement_info.push_back(std::move(origin_info));
    }

    callback.Run(std::move(engagement_info));
  }

 private:
  Profile* profile_;

  mojo::Binding<mojom::MediaEngagementDetailsProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementDetailsProviderImpl);
};

}  // namespace

MediaEngagementUI::MediaEngagementUI(content::WebUI* web_ui)
    : MojoWebUIController<mojom::MediaEngagementDetailsProvider>(web_ui) {
  // Setup the data source behind chrome://media-engagement.
  std::unique_ptr<content::WebUIDataSource> source(
      content::WebUIDataSource::Create(chrome::kChromeUIMediaEngagementHost));
  source->AddResourcePath("media-engagement.js", IDR_MEDIA_ENGAGEMENT_JS);
  source->AddResourcePath("chrome/browser/media/media_engagement_details.mojom",
                          IDR_MEDIA_ENGAGEMENT_MOJO_JS);
  source->AddResourcePath("url/mojo/url.mojom", IDR_URL_MOJO_JS);
  source->SetDefaultResource(IDR_MEDIA_ENGAGEMENT_HTML);
  source->UseGzip(std::unordered_set<std::string>());
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source.release());
}

MediaEngagementUI::~MediaEngagementUI() {}

void MediaEngagementUI::BindUIHandler(
    const service_manager::BindSourceInfo& source_info,
    mojom::MediaEngagementDetailsProviderRequest request) {
  ui_handler_.reset(new MediaEngagementDetailsProviderImpl(
      Profile::FromWebUI(web_ui()), std::move(request)));
}
