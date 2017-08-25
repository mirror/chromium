// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLLUX_WEBUI_POLLUX_WEBUI_HANDLER_H_
#define COMPONENTS_POLLUX_WEBUI_POLLUX_WEBUI_HANDLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/proximity_auth/pollux/assertion_session.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace pollux {

// Handles messages from the chrome://proximity-auth/pollux.html page.
class PolluxWebUIHandler : public content::WebUIMessageHandler,
                           public AssertionSession::Observer {
 public:
  explicit PolluxWebUIHandler();
  ~PolluxWebUIHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

 private:
  void CreateMasterKey(const base::ListValue* args);
  void DeriveCloudPairingKeys(const base::ListValue* args);
  void CreateNewChallenge(const base::ListValue* args);
  void GetAssertion(const base::ListValue* args);
  void StopAssertion(const base::ListValue* args);

  // AssertionSession::Observer:
  void OnStateChanged(AssertionSession::State old_state,
                      AssertionSession::State new_state) override;
  void OnGotAssertion(const std::string& assertion) override;
  void OnError(const std::string& error_message) override;

  std::unique_ptr<AssertionSession> assertion_session_;

  base::WeakPtrFactory<PolluxWebUIHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolluxWebUIHandler);
};

}  // namespace pollux

#endif  // COMPONENTS_POLLUX_WEBUI_POLLUX_WEBUI_HANDLER_H_
