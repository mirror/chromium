// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/singleton.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

// Downloads and deserializes the proto for the wallpaper collection names from
// the Backdrop service.
class CollectionNameFetcher : public net::URLFetcherDelegate {
 public:
  using OnCollectionNamesFetched =
      base::OnceCallback<void(const base::DictionaryValue& collection_names)>;

  CollectionNameFetcher();
  ~CollectionNameFetcher() override;

  // Triggers the start of the downloading and deserializing of the proto.
  void Start(OnCollectionNamesFetched on_collection_names_fetched);

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // Used to download the proto from the Backdrop service.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  OnCollectionNamesFetched on_collection_names_fetched_;

  DISALLOW_COPY_AND_ASSIGN(CollectionNameFetcher);
};

// Downloads and deserializes the proto for the wallpaper images info from the
// Backdrop service.
class ImageInfoFetcher : public net::URLFetcherDelegate {
 public:
  using OnImagesInfoFetched =
      base::OnceCallback<void(const base::ListValue& images_info)>;

  explicit ImageInfoFetcher(const std::string& collection_id);
  ~ImageInfoFetcher() override;

  // Triggers the start of the downloading and deserializing of the proto.
  void Start(OnImagesInfoFetched on_images_info_fetched);

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // Used to download the proto from the Backdrop service.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // The id of the collection, used as the token to fetch the images info.
  const std::string collection_id_;

  OnImagesInfoFetched on_images_info_fetched_;

  DISALLOW_COPY_AND_ASSIGN(ImageInfoFetcher);
};

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLERS_BACKDROP_WALLPAPER_HANDLERS_H_
