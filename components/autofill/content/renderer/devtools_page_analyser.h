// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_

#include "content/public/renderer/render_frame.h"

namespace autofill {

class DevToolsPageAnalyser {
 public:
  void AnalyseDocumentDOM(content::RenderFrame* render_frame);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_DEVTOOLS_PAGE_ANALYSER_H_
