// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_SUGGESTIONS_UI_INTERNALS_H_
#define COMPONENTS_SUGGESTIONS_SUGGESTIONS_UI_INTERNALS_H_

#include "base/barrier_closure.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "components/suggestions/suggestions_service.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace suggestions {

// SuggestionsSource renders a webpage to list SuggestionsService data.
class SuggestionsSource {
 public:
  SuggestionsSource(SuggestionsService* suggestions_service,
                    const std::string& base_url);
  ~SuggestionsSource();

  typedef base::Callback<void(scoped_refptr<base::RefCountedMemory>)>
      GotDataCallback;

  void StartDataRequest(const std::string& path,
                        const GotDataCallback& callback);
  std::string GetMimeType(const std::string& path) const;

 private:
  // Container for the state of a request.
  struct RequestContext {
    RequestContext(
        bool is_refresh_in,
        const suggestions::SuggestionsProfile& suggestions_profile_in,
        const GotDataCallback& callback_in);
    ~RequestContext();

    const bool is_refresh;
    const suggestions::SuggestionsProfile suggestions_profile;
    const GotDataCallback callback;
    std::map<GURL, std::string> base64_encoded_pngs;
  };

  // Callback for responses from each Thumbnail request.
  void OnThumbnailAvailable(RequestContext* context,
                            base::Closure barrier,
                            const GURL& url,
                            const gfx::Image& image);

  // Callback for when all requests are complete. Renders the output webpage and
  // passes the result to the original caller.
  void OnThumbnailsFetched(RequestContext* context);

  // Only used when servicing requests on the UI thread.
  SuggestionsService* suggestions_service_;

  // The base URL at which which the Suggestions WebUI lives in the context of
  // the embedder.
  std::string base_url_;

  // For callbacks may be run after destruction.
  base::WeakPtrFactory<SuggestionsSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionsSource);
};

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_SUGGESTIONS_UI_INTERNALS_H_
