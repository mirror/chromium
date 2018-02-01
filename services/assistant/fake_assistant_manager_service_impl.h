// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_
#define SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "services/assistant/assistant_manager_service.h"

namespace assistant {

class FakeAssistantManagerServiceImpl : public AssistantManagerService {
 public:
  FakeAssistantManagerServiceImpl();
  ~FakeAssistantManagerServiceImpl() override;

  // assistant::AssistantManagerService overrides
  void Start() override;

  DISALLOW_COPY_AND_ASSIGN(FakeAssistantManagerServiceImpl);
};

}  // namespace assistant

#endif  // SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_
