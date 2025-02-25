// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_
#define COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/update_client/component.h"
#include "url/gurl.h"

namespace update_client {

class Configurator;
class PersistedData;

class UpdateChecker {
 public:
  using UpdateCheckCallback =
      base::OnceCallback<void(int error, int retry_after_sec)>;

  using Factory = std::unique_ptr<UpdateChecker> (*)(
      const scoped_refptr<Configurator>& config,
      PersistedData* persistent);

  virtual ~UpdateChecker() = default;

  // Initiates an update check for the components specified by their ids.
  // |additional_attributes| provides a way to customize the <request> element.
  // This value is inserted as-is, therefore it must be well-formed as an
  // XML attribute string.
  // On completion, the state of |components| is mutated as required by the
  // server response received.
  virtual void CheckForUpdates(const std::string& session_id,
                               const std::vector<std::string>& ids_to_check,
                               const IdToComponentPtrMap& components,
                               const std::string& additional_attributes,
                               bool enabled_component_updates,
                               UpdateCheckCallback update_check_callback) = 0;

  static std::unique_ptr<UpdateChecker> Create(
      const scoped_refptr<Configurator>& config,
      PersistedData* persistent);

 protected:
  UpdateChecker() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(UpdateChecker);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_
