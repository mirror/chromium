// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/singleton.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class Value;
}

namespace net {
class URLRequestContextGetter;
}

class ImageInfoFetcher;

// Downloads and deserializes the proto from the Backdrop wallpaper service.
class BackdropWallpaperHandler : public net::URLFetcherDelegate {
 public:
  static BackdropWallpaperHandler* GetInstance();

  // Triggers the start of the downloading and deserializing of the proto.
  void Start();

  // Gets a list of wallpaper collection names (Art, Landscape etc.). Returns
  // false if the collection names are not available yet.
  bool GetCollectionNames(base::Value* collection_names_out);

  // Gets the info of the wallpapers belonging to the |collection_name|. Each
  // wallpaper is represented by a dictionary containing the image url, the
  // 'Learn more' url and the display texts (artist name, description etc.).
  // Returns false if the info for the collection is not available yet.
  bool GetImagesInfoPerCollection(const std::string& collection_name,
                                  base::Value* images_info_out);

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  friend struct base::DefaultSingletonTraits<BackdropWallpaperHandler>;

  BackdropWallpaperHandler();

  ~BackdropWallpaperHandler() override;

  // Used to initialize net::URLFetcher object.
  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;

  // Used to download the proto from the Backdrop service.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Maps each collection name with the image info fetcher that's responsible
  // for this collection.
  std::map<std::string, ImageInfoFetcher*> image_info_fetchers_;

  // A flag indicating if the collection names are ready to be fetched.
  bool collection_names_ready = false;

  DISALLOW_COPY_AND_ASSIGN(BackdropWallpaperHandler);
};

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_
