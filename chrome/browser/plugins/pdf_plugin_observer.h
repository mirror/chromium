// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PDF_PLUGIN_OBSERVER_H_
#define CHROME_BROWSER_PLUGINS_PDF_PLUGIN_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PDFPluginObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PDFPluginObserver> {
 public:
  ~PDFPluginObserver() override;

  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  friend class content::WebContentsUserData<PDFPluginObserver>;
  explicit PDFPluginObserver(content::WebContents* web_contents);
  void OnOpenPDF(content::RenderFrameHost* render_frame_host, const GURL& url);

  DISALLOW_COPY_AND_ASSIGN(PDFPluginObserver);
};

#endif  // CHROME_BROWSER_PLUGINS_PDF_PLUGIN_OBSERVER_H_
