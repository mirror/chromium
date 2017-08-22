// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_

#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace autofill {

// This class provides feedback to web developers about the password forms on
// their webpages, in order to increase the accessibility of web forms to the
// Password Manager. This is achieved by crawling the DOM whenever new forms are
// added to the page and checking for common mistakes, or ways in which the form
// could be improved (for example, with autocomplete attributes). See
// |AnalyseDocumentDOM| for more specific information about these warnings.
class DevToolsPagePasswordsAnalyser {
 public:
  DevToolsPagePasswordsAnalyser();

  ~DevToolsPagePasswordsAnalyser();

  void Reset();

  void AnalyseDocumentDOM(blink::WebLocalFrame* frame);

 private:
  std::set<blink::WebNode> skip_nodes_;
};

}  // namespace autofill

#endif
   // COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_
