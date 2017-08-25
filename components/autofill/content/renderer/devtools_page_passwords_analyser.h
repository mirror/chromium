// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_

#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {
class RenderFrame;
}

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

  // Clear the set of nodes that have already been analysed, so that they will
  // be analysed again next time |AnalyseDocumentDOM| is called. This is called
  // upon page load, for instance.
  void Reset();

  // If the DevTools are not attached, the DOM should not be analysed
  // immediately, to avoid a potentially-unnecessary DOM traversal. This
  // method indicates that the DOM should be analysed when the DevTools are next
  // opened, or immediately if they are already open.
  void MarkDOMForAnalysis(content::RenderFrame* render_frame);

  // Analyses the DOM if the document has been marked as dirty.
  void AnalyseDocumentDOMIfNeedBe(content::RenderFrame* render_frame);

 private:
  // |AnalyseDocumentDOM| traverses the DOM, logging potential issues in the
  // DevTools console. Errors are logged for those issues that conflict with the
  // HTML specification. Warnings are logged for issues that cause problems with
  // identification of fields on the web-page for the Password Manager.
  void AnalyseDocumentDOM(blink::WebLocalFrame* frame);

  std::set<blink::WebNode> skip_nodes_;
  // This is true when new DOM content is available since the last time
  // the page was analysed, meaning the page needs to be reanalysed.
  bool page_dirty_;
};

}  // namespace autofill

#endif
   // COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_PASSWORDS_ANALYSER_H_
