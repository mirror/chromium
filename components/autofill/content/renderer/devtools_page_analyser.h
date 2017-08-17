// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_

#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace autofill {

class DevToolsPageAnalyser {
 public:
  DevToolsPageAnalyser();

  ~DevToolsPageAnalyser();

  void Reset();

  void AnalyseDocumentDOM(blink::WebLocalFrame* frame);

 private:
  std::set<blink::WebNode> skip_nodes;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_
