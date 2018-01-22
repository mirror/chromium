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

namespace {

// The MIME type of the POST data sent to the server.
const char kProtoMimeType[] = "application/x-protobuf";

// The url to download the proto of the complete list of wallpaper collections.
const char kBackdropCollectionsUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collections?rt=b";

// The url to download the proto of a specific wallpaper collection.
const char kBackdropImagesUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collection-images?rt=b";

}  // namespace

// Downloads and deserializes the proto of a specific Backdrop wallpaper
// collection.
class ImageInfoFetcher : public net::URLFetcherDelegate {
 public:
  ImageInfoFetcher(const std::string& collection_id)
      : url_context_getter_(g_browser_process->system_request_context()),
        collection_id_(collection_id) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  ~ImageInfoFetcher() override = default;

  // Triggers the start of the downloading and deserializing of the proto.
  void Start() {
    url_fetcher_ = net::URLFetcher::Create(GURL(kBackdropImagesUrl),
                                           net::URLFetcher::POST, this);
    url_fetcher_->SetRequestContext(url_context_getter_.get());
    backdrop::GetImagesInCollectionRequest request;
    // TODO(crbug.com/800945): Supports all languages.
    request.set_language("en");
    request.set_collection_id(collection_id_);
    std::string serialized_proto;
    request.SerializeToString(&serialized_proto);
    url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
    url_fetcher_->Start();
  }

  // Provides the downloaded and deserialized info for the specific collection.
  // Returns false if the info is not available yet.
  bool GetImagesInfo(base::Value* images_info_out) {
    if (!images_info_ready_)
      return false;

    *images_info_out = images_info_->Clone();
    return true;
  }

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK(!images_info_ready_);
    if (!source->GetStatus().is_success() ||
        source->GetResponseCode() != net::HTTP_OK) {
      // TODO(crbug.com/800945): Adds retry mechanism and error handling.
      return;
    }
    std::string response_string;
    source->GetResponseAsString(&response_string);
    backdrop::GetImagesInCollectionResponse images_response;
    if (!images_response.ParseFromString(response_string))
      return;

    images_info_.reset(new base::Value(base::Value::Type::LIST));
    for (int i = 0; i < images_response.images_size(); ++i) {
      backdrop::Image image = images_response.images()[i];

      // The info of each image should contain image url, action url and display
      // text.
      base::Value image_info(base::Value::Type::DICTIONARY);
      image_info.SetKey("imageUrl", base::Value(image.image_url()));
      image_info.SetKey("actionUrl", base::Value(image.action_url()));

      // Display text may have more than one strings.
      base::Value display_text(base::Value::Type::LIST);
      for (int j = 0; j < image.attribution_size(); ++j) {
        display_text.GetList().push_back(
            base::Value(image.attribution()[j].text()));
      }
      image_info.SetKey("displayText", std::move(display_text));

      images_info_->GetList().push_back(std::move(image_info));
    }

    url_fetcher_.reset();
    images_info_ready_ = true;
  }

 private:
  // Used to initialize net::URLFetcher object.
  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;

  // Used to download the images info from the Backdrop service.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // The id of the collection, used as the token to fetch the images info.
  std::string collection_id_;

  // The list of images info.
  std::unique_ptr<base::Value> images_info_;

  // A flag indicating if the images info is ready to be fetched.
  bool images_info_ready_ = false;

  DISALLOW_COPY_AND_ASSIGN(ImageInfoFetcher);
};

BackdropWallpaperHandler* BackdropWallpaperHandler::GetInstance() {
  return base::Singleton<BackdropWallpaperHandler>::get();
}

void BackdropWallpaperHandler::Start() {
  url_fetcher_ = net::URLFetcher::Create(GURL(kBackdropCollectionsUrl),
                                         net::URLFetcher::POST, this);
  url_fetcher_->SetRequestContext(url_context_getter_.get());
  backdrop::GetCollectionsRequest request;
  // TODO(crbug.com/800945): Supports all languages.
  request.set_language("en");
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);
  url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
  url_fetcher_->Start();
}

bool BackdropWallpaperHandler::GetCollectionNames(
    base::Value* collection_names_out) {
  if (!collection_names_ready)
    return false;

  for (const auto& it : image_info_fetchers_)
    collection_names_out->GetList().push_back(base::Value(it.first));
  return true;
}

bool BackdropWallpaperHandler::GetImagesInfoPerCollection(
    const std::string& collection_name,
    base::Value* images_info_out) {
  // The collection name must already exist in the map.
  auto it = image_info_fetchers_.find(collection_name);
  DCHECK(it != image_info_fetchers_.end());

  base::Value images_info(base::Value::Type::LIST);
  if (it->second->GetImagesInfo(&images_info)) {
    *images_info_out = std::move(images_info);
    return true;
  }
  return false;
}

void BackdropWallpaperHandler::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(!collection_names_ready);
  if (!source->GetStatus().is_success() ||
      source->GetResponseCode() != net::HTTP_OK) {
    // TODO(crbug.com/800945): Adds retry mechanism and error handling.
    return;
  }

  std::string response_string;
  source->GetResponseAsString(&response_string);
  backdrop::GetCollectionsResponse collections_response;
  if (!collections_response.ParseFromString(response_string))
    return;

  for (int i = 0; i < collections_response.collections_size(); ++i) {
    backdrop::Collection collection = collections_response.collections(i);
    const std::string collection_name = collection.collection_name();
    DCHECK(image_info_fetchers_.find(collection_name) ==
           image_info_fetchers_.end());

    // Creates an ImageInfoFetcher object for each collection.
    ImageInfoFetcher* image_info_fetcher =
        new ImageInfoFetcher(collection.collection_id());
    image_info_fetchers_[collection_name] = image_info_fetcher;
    image_info_fetcher->Start();
  }

  url_fetcher_.reset();
  collection_names_ready = true;
}

BackdropWallpaperHandler::BackdropWallpaperHandler()
    : url_context_getter_(g_browser_process->system_request_context()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

BackdropWallpaperHandler::~BackdropWallpaperHandler() {
  for (const auto& it : image_info_fetchers_)
    delete it.second;
}
