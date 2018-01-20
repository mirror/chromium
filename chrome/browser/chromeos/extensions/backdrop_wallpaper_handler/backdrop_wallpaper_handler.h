// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_

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
}  // namespace net

class ImageInfoFetcher;

class BackdropWallpaperHandler : public net::URLFetcherDelegate {
 public:
  static BackdropWallpaperHandler* GetInstance();

  void Start();

  bool GetImagesInfo(base::Value* collection_name_out,
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

  std::vector<ImageInfoFetcher*> image_info_fetchers_;

  DISALLOW_COPY_AND_ASSIGN(BackdropWallpaperHandler);
};

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_BACKDROP_WALLPAPER_HANDLER_BACKDROP_WALLPAPER_HANDLER_H_