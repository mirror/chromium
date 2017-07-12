// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/passwords_private/passwords_private_utils.h"

#include <string>
#include <tuple>
#include <utility>

#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/password_ui_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace extensions {

api::passwords_private::UrlCollection CreateUrlCollectionFromForm(
    const autofill::PasswordForm& form) {
  std::string shown_origin;
  GURL link_url;
  std::tie(shown_origin, link_url) =
      password_manager::GetShownOriginAndLinkUrl(form);

  api::passwords_private::UrlCollection urls;
  urls.origin = form.signon_realm;
  urls.shown = std::move(shown_origin);
  urls.link = link_url.spec();

  if (password_manager::IsValidAndroidFacetURI(form.signon_realm)) {
    // Currently we use "direction=rtl" in CSS to elide long origins from the
    // left. This does not play nice with appending strings that end in
    // punctuation symbols, which is why the bidirectional override tag is
    // necessary.
    // TODO(crbug.com/679434): Clean this up.
    // Reference:
    // https://www.w3.org/International/questions/qa-bidi-unicode-controls
    urls.shown += "\u202D" +  // equivalent to <bdo dir = "ltr">
                  l10n_util::GetStringUTF8(IDS_PASSWORDS_ANDROID_URI_SUFFIX) +
                  "\u202C";  // equivalent to </bdo>
  }

  return urls;
}

}  // namespace extensions
