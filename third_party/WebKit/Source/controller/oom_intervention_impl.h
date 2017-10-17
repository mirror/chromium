// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OomInterventionImpl_h
#define OomInterventionImpl_h

#include "third_party/WebKit/public/platform/oom_intervention.mojom.h"

namespace blink {

class LocalFrame;
class ScopedPageSuspender;

class OomInterventionImpl : public mojom::OomIntervention {
 public:
  static void Create(LocalFrame*, mojom::OomInterventionRequest request);

  OomInterventionImpl(LocalFrame*);
  ~OomInterventionImpl() override;

  // mojom::OomIntervention implementations:
  void OnNearOomDetected() override;
  void OnInterventionDeclined() override;

 private:
  void Suspend();

  std::unique_ptr<ScopedPageSuspender> suspender_;
};

}  // namespace blink

#endif  // OomInterventionImpl_h
