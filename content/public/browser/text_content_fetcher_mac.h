// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_TEXT_CONTENT_FETCHER_MAC_H_
#define CONTENT_PUBLIC_BROWSER_TEXT_CONTENT_FETCHER_MAC_H_


#include <memory>
#include <string>

#include "base/callback.h"
#include "ui/gfx/range/range.h"

namespace content {

// The interface to retrieve provision information for CDM.
class TextContentFetcher {
 public:

  //
  using ResponseCB =
      base::Callback<void(gfx::Range edit_range, const std::string& text)>;

  virtual ~TextContentFetcher() {}

  virtual void GetTextForSuggestions(const ResponseCB& response_cb) = 0;
};

using CreateFetcherCB = base::Callback<std::unique_ptr<TextContentFetcher>()>;

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_TEXT_CONTENT_FETCHER_MAC_H_
