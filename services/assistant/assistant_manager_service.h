// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_H_
#define SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_H_

#include <memory>
#include <string>

namespace assistant {

class AssistantManagerService {
 public:
  class AuthTokenProvider {
   public:
    virtual std::string& GetAccessToken();

   protected:
    virtual ~AuthTokenProvider() = default;
  };

  virtual void Start() = 0;

 protected:
  virtual ~AssistantManagerService() = default;
};

}  // namespace assistant

#endif  // SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_H_
