// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/backdrop_wallpaper_handlers/backdrop_wallpaper_handlers.h"

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/extensions/backdrop_wallpaper_handlers/backdrop_wallpaper.pb.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace {

// The MIME type of the POST data sent to the server.
constexpr char kProtoMimeType[] = "application/x-protobuf";

// The url to download the proto of the complete list of wallpaper collections.
constexpr char kBackdropCollectionsUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collections?rt=b";

// The url to download the proto of a specific wallpaper collection.
constexpr char kBackdropImagesUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collection-images?rt=b";

}  // namespace

CollectionNameFetcher::CollectionNameFetcher() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

CollectionNameFetcher::~CollectionNameFetcher() = default;

void CollectionNameFetcher::Start(
    OnCollectionNamesFetched on_collection_names_fetched) {
  // Cancel any in-flight requests.
  url_fetcher_.reset();
  on_collection_names_fetched_ = std::move(on_collection_names_fetched);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("backdrop_collection_names_download",
                                          R"(
        semantics {
          sender: "ChromeOS Wallpaper Picker"
          description:
            "The ChromeOS Wallpaper Picker extension displays a rich set of "
            "wallpapers for users to choose from. Each wallpaper belongs to a "
            "collection (e.g. Arts, Landscape etc.). The list of all available "
            "collections is downloaded from the Backdrop wallpaper service."
          trigger:
            "When ChromeOS Wallpaper Picker extension is open, and "
            "GOOGLE_CHROME_BUILD is defined."
          data:
            "The Backdrop protocol buffer messages. No user data is included."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "NA"
          policy_exception_justification:
            "Not implemented, considered not necessary."
        })");

  url_fetcher_ =
      net::URLFetcher::Create(GURL(kBackdropCollectionsUrl),
                              net::URLFetcher::POST, this, traffic_annotation);
  url_fetcher_->SetRequestContext(g_browser_process->system_request_context());
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);

  backdrop::GetCollectionsRequest request;
  // TODO(crbug.com/800945): Supports all languages.
  request.set_language("en");
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);
  url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
  url_fetcher_->Start();
}

void CollectionNameFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  if (!source->GetStatus().is_success() ||
      source->GetResponseCode() != net::HTTP_OK) {
    // TODO(crbug.com/800945): Adds retry mechanism and error handling.
    LOG(ERROR) << "Downloading Backdrop wallpaper proto for collection names "
                  "failed with error code: "
               << source->GetResponseCode();
    std::move(on_collection_names_fetched_).Run(base::DictionaryValue());
    return;
  }

  std::string response_string;
  source->GetResponseAsString(&response_string);
  backdrop::GetCollectionsResponse collections_response;
  if (!collections_response.ParseFromString(response_string)) {
    LOG(ERROR) << "Deserializing Backdrop wallpaper proto for collection names "
                  "failed.";
    std::move(on_collection_names_fetched_).Run(base::DictionaryValue());
    return;
  }

  base::DictionaryValue collection_names;
  for (int i = 0; i < collections_response.collections_size(); ++i) {
    backdrop::Collection collection = collections_response.collections(i);
    collection_names.SetString(collection.collection_id(),
                               collection.collection_name());
  }

  url_fetcher_.reset();
  std::move(on_collection_names_fetched_).Run(collection_names);
}

ImageInfoFetcher::ImageInfoFetcher(const std::string& collection_id)
    : collection_id_(collection_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ImageInfoFetcher::~ImageInfoFetcher() = default;

void ImageInfoFetcher::Start(OnImagesInfoFetched on_images_info_fetched) {
  // Cancel any in-flight requests.
  url_fetcher_.reset();
  on_images_info_fetched_ = std::move(on_images_info_fetched);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("backdrop_images_info_download", R"(
        semantics {
          sender: "ChromeOS Wallpaper Picker"
          description:
            "When user clicks on a particular wallpaper collection on the "
            "ChromeOS Wallpaper Picker, it displays the preview of the iamges "
            "and descriptive texts for each image. Such information is "
            "downloaded from the Backdrop wallpaper service."
          trigger:
            "When ChromeOS Wallpaper Picker extension is open, "
            "GOOGLE_CHROME_BUILD is defined and user clicks on a particular "
            "collection."
          data:
            "The Backdrop protocol buffer messages. No user data is included."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "NA"
          policy_exception_justification:
            "Not implemented, considered not necessary."
        })");

  url_fetcher_ =
      net::URLFetcher::Create(GURL(kBackdropImagesUrl), net::URLFetcher::POST,
                              this, traffic_annotation);
  url_fetcher_->SetRequestContext(g_browser_process->system_request_context());
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);

  backdrop::GetImagesInCollectionRequest request;
  // TODO(crbug.com/800945): Supports all languages.
  request.set_language("en");
  request.set_collection_id(collection_id_);
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);
  url_fetcher_->SetUploadData(kProtoMimeType, serialized_proto);
  url_fetcher_->Start();
}

void ImageInfoFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  if (!source->GetStatus().is_success() ||
      source->GetResponseCode() != net::HTTP_OK) {
    // TODO(crbug.com/800945): Adds retry mechanism and error handling.
    LOG(ERROR) << "Downloading Backdrop wallpaper proto for collection "
               << collection_id_
               << " failed with error code: " << source->GetResponseCode();
    std::move(on_images_info_fetched_).Run(base::ListValue());
    return;
  }
  std::string response_string;
  source->GetResponseAsString(&response_string);
  backdrop::GetImagesInCollectionResponse images_response;
  if (!images_response.ParseFromString(response_string)) {
    LOG(ERROR) << "Deserializing Backdrop wallpaper proto for collection "
               << collection_id_ << " failed";
    std::move(on_images_info_fetched_).Run(base::ListValue());
    return;
  }

  base::ListValue images_info_list;
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
      display_text.GetList().emplace_back(
          base::Value(image.attribution()[j].text()));
    }
    image_info.SetKey("displayText", std::move(display_text));

    images_info_list.GetList().push_back(std::move(image_info));
  }

  url_fetcher_.reset();
  std::move(on_images_info_fetched_).Run(images_info_list);
}
