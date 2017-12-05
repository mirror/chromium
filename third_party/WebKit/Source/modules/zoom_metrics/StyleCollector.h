// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StyleCollector_h
#define StyleCollector_h

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "platform/heap/Persistent.h"
#include "public/platform/modules/zoom_metrics/style_collector.mojom-blink.h"

namespace blink {

class LocalFrame;

namespace zoom_metrics {

// Provides style information for the given frame that is pertinent to the
// user's choice of zoom level for the page.
class StyleCollector final : public mojom::zoom_metrics::blink::StyleCollector {
 public:
  explicit StyleCollector(const LocalFrame&);

  static void BindMojoRequest(
      const LocalFrame*,
      mojom::zoom_metrics::blink::StyleCollectorRequest);

  // mojom::zoom_metrics::blink::StyleCollector:
  // Note that this is run as an idle task.
  void GetStyle(GetStyleCallback) override;

  void GetStyleImmediatelyForTesting(
      GetStyleImmediatelyForTestingCallback) override;

 private:
  void GetStyleImmediately(GetStyleCallback, double deadline_seconds);

  WeakPersistent<const LocalFrame> frame_;
  base::WeakPtrFactory<StyleCollector> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(StyleCollector);
};

}  // namespace zoom_metrics
}  // namespace blink

#endif  // StyleCollector_h
