// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/data_reduction_proxy/core/common/lofi_ui_service.h"

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace content {
class WebContents;
}

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

using OnPreviewsShownCallback =
    base::Callback<void(content::WebContents* web_contents,
                        previews::PreviewsType type,
                        const GURL& non_preview_url)>;

// Passes notifications to the UI thread that a Lo-Fi response has been
// received. These notifications may be used to show Lo-Fi UI. This object lives
// on the IO thread and OnLoFiReponseReceived should be called from there.
class ContentLoFiUIService : public LoFiUIService {
 public:
  ContentLoFiUIService(
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const OnPreviewsShownCallback& on_previews_shown_callback);
  ~ContentLoFiUIService() override;

  // LoFiUIService implementation:
  void OnPreviewsShown(const net::URLRequest& request,
                       previews::PreviewsType type,
                       const GURL& non_preview_url) override;

 private:
  // Using the |render_process_id| and |render_frame_id|, gets the associated
  // WebContents if it exists and runs the
  // |notify_previews_shown_callback_|.
  void OnPreviewsShownOnUIThread(int render_process_id,
                                 int render_frame_id,
                                 previews::PreviewsType type,
                                 const GURL& non_preview_url);

  // A task runner to post calls to OnLoFiReponseReceivedOnUI on the UI
  // thread.
  const scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  const OnPreviewsShownCallback on_previews_shown_callback_;

  DISALLOW_COPY_AND_ASSIGN(ContentLoFiUIService);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_
