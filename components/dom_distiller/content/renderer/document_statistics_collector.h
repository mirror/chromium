// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DOCUMENT_STATISTICS_COLLECTOR_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DOCUMENT_STATISTICS_COLLECTOR_H_

namespace webagents {
class Document;
}

namespace dom_distiller {

struct WebDistillabilityFeatures;

class DocumentStatisticsCollector {
 public:
  static WebDistillabilityFeatures CollectStatistics(webagents::Document);
};

}  // namespace dom_distiller

#endif  //  COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DOCUMENT_STATISTICS_COLLECTOR_H_
