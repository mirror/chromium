// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/backdrop_wallpaper_handler/backdrop_wallpaper_handler.h"

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "components/wallpaper/backdrop_wallpaper.pb.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

namespace {

// The MIME type of the POST data sent to the server.
const char kProtoMimeType[] = "application/x-protobuf";

const char kBackdropCollectionsUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collections?rt=b";
const char kBackdropImagesUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collection-images?rt=b";

}  // namespace

class ImageInfoFetcher : public net::URLFetcherDelegate {
 public:
  ImageInfoFetcher(const std::string collection_id,
                   const std::string collection_name)
      : url_context_getter_(g_browser_process->system_request_context()),
        collection_id_(collection_id),
        collection_name_(collection_name) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  ~ImageInfoFetcher() override = default;

  void Start() {
    url_fetcher_ = net::URLFetcher::Create(GURL(kBackdropImagesUrl),
                                           net::URLFetcher::POST, this);
    url_fetcher_->SetRequestContext(url_context_getter_.get());
    backdrop::GetImagesInCollectionRequest request;
    request.set_language("en");
    request.set_collection_id(collection_id_);
    std::string serialized_proto;
    request.SerializeToString(&serialized_proto);
    url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
    url_fetcher_->Start();
  }

  const std::string& collection_name() { return collection_name_; }

  bool GetImagesInfoForCollection(base::Value* images_info_out) {
    if (!images_info_available_)
      return false;

    *images_info_out = std::move(*images_info_);
    images_info_available_ = false;
    return true;
  }

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK(!images_info_available_);
    if (!source->GetStatus().is_success() ||
        source->GetResponseCode() != net::HTTP_OK) {
      return;
    }
    std::string response_string;
    source->GetResponseAsString(&response_string);
    url_fetcher_.reset();

    backdrop::GetImagesInCollectionResponse images_response;
    if (!images_response.ParseFromString(response_string))
      return;

    images_info_.reset(new base::Value(base::Value::Type::LIST));
    for (int i = 0; i < images_response.images_size(); ++i) {
      backdrop::Image image = images_response.images()[i];
      base::Value image_info(base::Value::Type::DICTIONARY);
      image_info.SetKey("imageUrl", base::Value(image.image_url()));
      image_info.SetKey("actionUrl", base::Value(image.action_url()));

      base::Value display_text(base::Value::Type::LIST);

      // LOG(ERROR) << "\n" + collection_name_;
      // LOG(ERROR) << image.image_url();
      // LOG(ERROR) << image.action_url();
      for (int j = 0; j < image.attribution_size(); ++j) {
        display_text.GetList().push_back(
            base::Value(image.attribution()[j].text()));
        // LOG(ERROR) << image.attribution()[j].text();
      }
      image_info.SetKey("displayText", std::move(display_text));

      images_info_->GetList().push_back(std::move(image_info));
    }
    images_info_available_ = true;
  }

 private:
  // Used to initialize net::URLFetcher object.
  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;

  // Used to download the images info from the Backdrop service.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  std::string collection_id_;
  std::string collection_name_;

  std::unique_ptr<base::Value> images_info_;

  bool images_info_available_ = false;

  DISALLOW_COPY_AND_ASSIGN(ImageInfoFetcher);
};

BackdropWallpaperHandler* BackdropWallpaperHandler::GetInstance() {
  return base::Singleton<BackdropWallpaperHandler>::get();
}

void BackdropWallpaperHandler::Start() {
  WallpaperControllerClient::Get()->Log("34");
  url_fetcher_ = net::URLFetcher::Create(GURL(kBackdropCollectionsUrl),
                                         net::URLFetcher::POST, this);
  url_fetcher_->SetRequestContext(url_context_getter_.get());
  backdrop::GetCollectionsRequest request;
  // TODO(crbug.com/800945): Support all languages.
  request.set_language("en");
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);
  url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
  url_fetcher_->Start();
  WallpaperControllerClient::Get()->Log("46");
}

bool BackdropWallpaperHandler::GetImagesInfo(base::Value* collection_name_out,
                                             base::Value* images_info_out) {
  for (ImageInfoFetcher* image_info_fetcher : image_info_fetchers_) {
    base::Value images_info(base::Value::Type::LIST);
    if (image_info_fetcher->GetImagesInfoForCollection(&images_info)) {
      *collection_name_out = base::Value(image_info_fetcher->collection_name());
      *images_info_out = std::move(images_info);
      LOG(ERROR) << "REACH 144";
      return true;
    }
  }
  LOG(ERROR) << "REACH 148";
  return false;
}

void BackdropWallpaperHandler::OnURLFetchComplete(
    const net::URLFetcher* source) {
  if (!source->GetStatus().is_success() ||
      source->GetResponseCode() != net::HTTP_OK) {
    // TODO(crbug.com/800945): Add retry mechanism and error handling.
    return;
  }
  std::string response_string;
  source->GetResponseAsString(&response_string);
  url_fetcher_.reset();

  backdrop::GetCollectionsResponse collections_response;
  if (!collections_response.ParseFromString(response_string))
    return;

  for (int i = 0; i < collections_response.collections_size(); ++i) {
    backdrop::Collection collection = collections_response.collections(i);
    ImageInfoFetcher* image_info_fetcher = new ImageInfoFetcher(
        collection.collection_id(), collection.collection_name());
    image_info_fetcher->Start();
    image_info_fetchers_.push_back(image_info_fetcher);
  }
}

BackdropWallpaperHandler::BackdropWallpaperHandler()
    : url_context_getter_(g_browser_process->system_request_context()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

BackdropWallpaperHandler::~BackdropWallpaperHandler() {
  for (ImageInfoFetcher* image_info_fetcher : image_info_fetchers_)
    delete image_info_fetcher;
}