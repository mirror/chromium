// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_EXTENSIONS_FAKE_ARC_SUPPORT_H_
#define CHROME_BROWSER_CHROMEOS_ARC_EXTENSIONS_FAKE_ARC_SUPPORT_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/arc/arc_support_host.h"
#include "extensions/browser/api/messaging/native_message_host.h"

namespace arc {

// Fake implementation of ARC support Chrome App for testing.
class FakeArcSupport : public extensions::NativeMessageHost::Client {
 public:
  using PageClosedCallback = base::Callback<void(ArcSupportHost::UIPage page)>;

  explicit FakeArcSupport(ArcSupportHost* support_host);
  ~FakeArcSupport() override;

  // Emulates to open ARC support Chrome app, and connect message host to
  // ARC support host.
  void Open(Profile* profile);

  // Emulates clicking Close button.
  void Close();

  // Authentication page emulation (either LSO or Active Directory).
  void EmulateAuthSuccess(const std::string& auth_code);
  void EmulateAuthFailure(const std::string& error_msg);

  // Emulates clicking Agree button on the fake terms of service page.
  void ClickAgreeButton();

  // Emulates clicking Cancel button on the fake Active Directory auth page.
  void ClickAdAuthCancelButton();

  bool metrics_mode() const { return metrics_mode_; }
  bool backup_and_restore_mode() const { return backup_and_restore_mode_; }
  bool location_service_mode() const { return location_service_mode_; }

  // Emulates checking preference box.
  void set_metrics_mode(bool mode) { metrics_mode_ = mode; }
  void set_backup_and_restore_mode(bool mode) {
    backup_and_restore_mode_ = mode;
  }
  void set_location_service_mode(bool mode) { location_service_mode_ = mode; }

  // Emulate setting the Active Directory auth federation URL.
  void set_ad_auth_federation_url(const std::string& federation_url) {
    ad_auth_federation_url_ = federation_url;
  }

  const std::string& get_ad_auth_federation_url() const {
    return ad_auth_federation_url_;
  }

  // Emulate setting the Active Directory DM server URL prefix.
  void set_dm_url_prefix(const std::string& dm_url_prefix) {
    ad_auth_dm_url_prefix_ = dm_url_prefix;
  }

  const std::string& get_ad_auth_dm_url_prefix() const {
    return ad_auth_dm_url_prefix_;
  }

  // Gets called when the UI page changes.
  void set_on_page_changed(const PageClosedCallback& callback) {
    on_page_changed_ = callback;
  }

  // Error page emulation.
  void ClickRetryButton();
  void ClickSendFeedbackButton();

  // Returns the current page.
  ArcSupportHost::UIPage ui_page() const { return ui_page_; }

 private:
  void UnsetMessageHost();

  // extensions::NativeMessageHost::Client:
  void PostMessageFromNativeHost(const std::string& message) override;
  void CloseChannel(const std::string& error_message) override;

  ArcSupportHost* const support_host_;

  std::unique_ptr<extensions::NativeMessageHost> native_message_host_;
  ArcSupportHost::UIPage ui_page_ = ArcSupportHost::UIPage::NO_PAGE;
  bool metrics_mode_ = false;
  bool backup_and_restore_mode_ = false;
  bool location_service_mode_ = false;
  std::string ad_auth_federation_url_;
  std::string ad_auth_dm_url_prefix_;
  PageClosedCallback on_page_changed_;

  base::WeakPtrFactory<FakeArcSupport> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeArcSupport);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_EXTENSIONS_FAKE_ARC_SUPPORT_H_
