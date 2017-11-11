// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/common/url_utils.h"

namespace autofill {

GURL StripAuthAndParams(const GURL& document_url) {
  GURL::Replacements rep;
  rep.ClearUsername();
  rep.ClearPassword();
  rep.ClearQuery();
  rep.ClearRef();
  return document_url.ReplaceComponents(rep);
}

}  // namespace autofill
