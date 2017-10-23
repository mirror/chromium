// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OomInterventionImpl_h
#define OomInterventionImpl_h

#include "public/platform/oom_intervention.mojom.h"

#include "public/platform/InterfaceRegistry.h"

namespace blink {

class ScopedPagePauser;

class OomInterventionImpl : public mojom::OomIntervention {
 public:
  static void RegisterInterface(InterfaceRegistry*);

  static void Create(mojom::OomInterventionRequest request);

  OomInterventionImpl();
  ~OomInterventionImpl() override;

  // mojom::OomIntervention implementations:
  void OnNearOomDetected() override;
  void OnInterventionDeclined() override;

 private:
  std::unique_ptr<ScopedPagePauser> pauser_;
};

}  // namespace blink

#endif  // OomInterventionImpl_h
