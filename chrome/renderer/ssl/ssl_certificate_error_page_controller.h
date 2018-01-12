// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_
#define CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "components/security_interstitials/core/controller_client.h"
#include "content/public/renderer/render_frame.h"
#include "gin/wrappable.h"

// This class makes various helper functions available to interstitials
// when committed interstitials are on. It is bound to the JavaScript
// window.certificateErrorPageController object.
class SSLCertificateErrorPageController
    : public gin::Wrappable<SSLCertificateErrorPageController> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  class Delegate {
   public:
    virtual void SendCommand(
        security_interstitials::SecurityInterstitialCommand command) = 0;

   protected:
    Delegate();
    virtual ~Delegate();

    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  // Will invoke methods on |delegate| in response to user actions taken on the
  // interstitial. May call delegate methods even after the page has been
  // navigated away from, so it is recommended consumers make sure the weak
  // pointers are destroyed in response to navigations.
  static void Install(content::RenderFrame* render_frame,
                      base::WeakPtr<Delegate> delegate);

 private:
  explicit SSLCertificateErrorPageController(base::WeakPtr<Delegate> delegate);
  ~SSLCertificateErrorPageController() override;

  bool DontProceed();
  bool Proceed();
  bool ShowMoreSection();
  bool OpenHelpCenter();
  bool OpenDiagnostic();
  bool Reload();
  bool OpenDateSettings();
  bool OpenLogin();
  bool DoReport();
  bool DontReport();
  bool OpenReportingPrivacy();
  bool OpenWhitepaper();
  bool ReportPhishingError();

  bool SendCommand(security_interstitials::SecurityInterstitialCommand command);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  base::WeakPtr<Delegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(SSLCertificateErrorPageController);
};

#endif  // CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_
