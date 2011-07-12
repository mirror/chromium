// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
#define CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
#pragma once

#include "base/compiler_specific.h"
#include "content/browser/content_browser_client.h"

class QuotaPermissionContext;

namespace chrome {

class ChromeContentBrowserClient : public content::ContentBrowserClient {
 public:
  virtual void RenderViewHostCreated(RenderViewHost* render_view_host) OVERRIDE;
  virtual void BrowserRenderProcessHostCreated(
      BrowserRenderProcessHost* host) OVERRIDE;
  virtual void PluginProcessHostCreated(PluginProcessHost* host) OVERRIDE;
  virtual void WorkerProcessHostCreated(WorkerProcessHost* host) OVERRIDE;
  virtual content::WebUIFactory* GetWebUIFactory() OVERRIDE;
  virtual bool ShouldUseProcessPerSite(Profile* profile,
                                       const GURL& effective_url) OVERRIDE;
  virtual GURL GetEffectiveURL(Profile* profile, const GURL& url) OVERRIDE;
  virtual bool IsURLSameAsAnySiteInstance(const GURL& url) OVERRIDE;
  virtual std::string GetCanonicalEncodingNameByAliasName(
      const std::string& alias_name) OVERRIDE;
  virtual void AppendExtraCommandLineSwitches(CommandLine* command_line,
                                              int child_process_id) OVERRIDE;
  virtual std::string GetApplicationLocale() OVERRIDE;
  virtual std::string GetAcceptLangs(const TabContents* tab) OVERRIDE;
  virtual SkBitmap* GetDefaultFavicon() OVERRIDE;
  virtual bool AllowAppCache(const GURL& manifest_url,
                             const content::ResourceContext& context) OVERRIDE;
  virtual bool AllowGetCookie(const GURL& url,
                              const GURL& first_party,
                              const net::CookieList& cookie_list,
                              const content::ResourceContext& context,
                              int render_process_id,
                              int render_view_id) OVERRIDE;
  virtual bool AllowSetCookie(const GURL& url,
                              const GURL& first_party,
                              const std::string& cookie_line,
                              const content::ResourceContext& context,
                              int render_process_id,
                              int render_view_id,
                              net::CookieOptions* options) OVERRIDE;
  virtual bool AllowSaveLocalState(
      const content::ResourceContext& context) OVERRIDE;
  virtual net::URLRequestContext* OverrideRequestContextForURL(
      const GURL& url, const content::ResourceContext& context) OVERRIDE;
  virtual QuotaPermissionContext* CreateQuotaPermissionContext() OVERRIDE;
  virtual void RevealFolderInOS(const FilePath& path) OVERRIDE;
  virtual void AllowCertificateError(
      SSLCertErrorHandler* handler,
      bool overridable,
      Callback2<SSLCertErrorHandler*, bool>::Type* callback) OVERRIDE;
  virtual void ShowClientCertificateRequestDialog(
      int render_process_id,
      int render_view_id,
      SSLClientAuthHandler* handler) OVERRIDE;
  virtual void AddNewCertificate(
      net::URLRequest* request,
      net::X509Certificate* cert,
      int render_process_id,
      int render_view_id) OVERRIDE;
  virtual void RequestDesktopNotificationPermission(
      const GURL& source_origin,
      int callback_context,
      int render_process_id,
      int render_view_id) OVERRIDE;
  virtual WebKit::WebNotificationPresenter::Permission
      CheckDesktopNotificationPermission(
          const GURL& source_url,
          const content::ResourceContext& context) OVERRIDE;
  virtual void ShowDesktopNotification(
      const DesktopNotificationHostMsg_Show_Params& params,
      int render_process_id,
      int render_view_id,
      bool worker) OVERRIDE;
  virtual void CancelDesktopNotification(
      int render_process_id,
      int render_view_id,
      int notification_id) OVERRIDE;
  virtual bool CanCreateWindow(
      const GURL& source_url,
      WindowContainerType container_type,
      const content::ResourceContext& context) OVERRIDE;
  virtual std::string GetWorkerProcessTitle(
      const GURL& url, const content::ResourceContext& context) OVERRIDE;
  virtual ResourceDispatcherHost* GetResourceDispatcherHost() OVERRIDE;
  virtual ui::Clipboard* GetClipboard() OVERRIDE;
  virtual bool IsFastShutdownPossible() OVERRIDE;
  virtual WebPreferences GetWebkitPrefs(Profile* profile,
                                        bool is_web_ui) OVERRIDE;
  virtual void UpdateInspectorSetting(RenderViewHost* rvh,
                                      const std::string& key,
                                      const std::string& value) OVERRIDE;
  virtual void ClearInspectorSettings(RenderViewHost* rvh) OVERRIDE;

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // Can return an optional fd for crash handling, otherwise returns -1.
  virtual int GetCrashSignalFD(const std::string& process_type) OVERRIDE;
#endif
#if defined(USE_NSS)
  virtual
      crypto::CryptoModuleBlockingPasswordDelegate* GetCryptoPasswordDelegate(
          const GURL& url) OVERRIDE;
#endif
};

}  // namespace chrome

#endif  // CHROME_BROWSER_CHROME_CONTENT_BROWSER_CLIENT_H_
