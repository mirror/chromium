// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_

#include "components/previews/core/previews_experiments.h"

namespace net {
class URLRequest;
}

namespace previews {

struct Hints {
  Hints();
  ~Hints();

  // TODO(crbug.com/77892): Define this.
};

class PreviewsOptimizationGuide {
 public:
  PreviewsOptimizationGuide();

  ~PreviewsOptimizationGuide();

  void SwapHints(const Hints& hints);

  // Returns whether |type| is whitelisted for |request|.
  bool IsWhitelisted(const net::URLRequest& request, PreviewsType type);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_
