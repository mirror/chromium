// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_H_

#include <string>

#include "base/values.h"

class Browser;
class Profile;

namespace content {
class RenderFrameHost;
class WebContents;
class WebUI;
}

namespace signin {

// Argument for |CanOfferSignin|.
enum CanOfferSigninType {
  CAN_OFFER_SIGNIN_FOR_ALL_ACCOUNTS,
  CAN_OFFER_SIGNIN_FOR_SECONDARY_ACCOUNT
};

// Returns true if sign-in is allowed for account with |email| and |gaia_id| to
// |profile|. If the sign-in is not allowed, then the error message is passed
// to the called in |out_error_message|
bool CanOfferSignin(Profile* profile,
                    CanOfferSigninType can_offer_type,
                    const std::string& gaia_id,
                    const std::string& email,
                    std::string* out_error_message);

// Return true if the account given by |email| and |gaia_id| is signed in to
// Chrome in a different profile.
bool IsCrossAccountError(Profile* profile,
                         const std::string& email,
                         const std::string& gaia_id);

// Gets a webview within an auth page that has the specified parent frame name
// (i.e. <webview name="foobar"></webview>).
content::RenderFrameHost* GetAuthFrame(content::WebContents* web_contents,
                                       const std::string& parent_frame_name);

content::WebContents* GetAuthFrameWebContents(
    content::WebContents* web_contents,
    const std::string& parent_frame_name);

// Gets the browser containing the web UI; if none is found, returns the last
// active browser for web UI's profile.
Browser* GetDesktopBrowser(content::WebUI* web_ui);

// Sets the height of the WebUI modal dialog after its initialization. This is
// needed to better accomodate different locales' text heights.
void SetInitializedModalHeight(Browser* browser,
                               content::WebUI* web_ui,
                               const base::ListValue* args);

}  // namespace signin

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_H_
