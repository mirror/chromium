// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/pdf_plugin_placeholder_observer.h"

#include "chrome/common/render_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/render_frame_host.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ppapi/features/features.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PDFPluginPlaceholderObserver);

PDFPluginPlaceholderObserver::PDFPluginPlaceholderObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_ptr_factory_(this) {}

PDFPluginPlaceholderObserver::~PDFPluginPlaceholderObserver() {}

bool PDFPluginPlaceholderObserver::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PDFPluginPlaceholderObserver, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_OpenPDF, OnOpenPDF)
    IPC_MESSAGE_UNHANDLED(return false)
  IPC_END_MESSAGE_MAP()

  return true;
}

void PDFPluginPlaceholderObserver::OnDownloadStarted(
    content::DownloadItem* item,
    content::DownloadInterruptReason interrupt_reason) {
  if (item && interrupt_reason == content::DOWNLOAD_INTERRUPT_REASON_NONE)
    item->SetOpenWhenComplete(true);
}

void PDFPluginPlaceholderObserver::OnOpenPDF(
    content::RenderFrameHost* render_frame_host,
    const GURL& url) {
  if (!content::ChildProcessSecurityPolicy::GetInstance()->CanRequestURL(
          render_frame_host->GetRoutingID(), url)) {
    return;
  }

#if BUILDFLAG(ENABLE_PLUGINS)
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("pdf_plugin_placeholder", R"(
        semantics {
          sender: "PDF Plugin Placeholder"
          description:
            "When the PDF Viewer is unavailable, a placeholder is shown for "
            "embedded PDFs. This placeholder allows the user to download and "
            "open the PDF file via a button."
          trigger:
            "The user clicks the 'View PDF' button in the PDF placeholder."
          data: "None."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature can be disabled via 'Download PDF files instead of "
            "automatically opening them in Chrome' in settings under content. "
            "The feature is disabled by default."
          chrome_policy {
            AlwaysOpenPdfExternally {
              AlwaysOpenPdfExternally: false
            }
          }
        })");
  auto params = content::DownloadUrlParameters::CreateForWebContentsMainFrame(
      web_contents(), url, traffic_annotation);
  params->set_callback(
      base::Bind(&PDFPluginPlaceholderObserver::OnDownloadStarted,
                 weak_ptr_factory_.GetWeakPtr()));
  content::BrowserContext::GetDownloadManager(
      web_contents()->GetBrowserContext())
      ->DownloadUrl(std::move(params));
#else
  content::OpenURLParams open_url_params(
      url,
      content::Referrer::SanitizeForRequest(
          url, content::Referrer(web_contents()->GetURL(),
                                 blink::kWebReferrerPolicyDefault)),
      WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_AUTO_BOOKMARK,
      false);
  open_url_params.user_gesture = true;
  web_contents()->OpenURL(open_url_params);
#endif
}
