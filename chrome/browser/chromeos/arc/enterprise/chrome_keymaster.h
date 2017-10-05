// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_CHROME_KEYMASTER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_CHROME_KEYMASTER_H_

#include <stdint.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/arc/common/cert_store.mojom.h"
#include "net/cert/x509_certificate.h"

namespace arc {

struct OperationInfo;

/**
 * This class performs Chrome keymaster operations.
 */
class ChromeKeymaster {
 public:
  ChromeKeymaster();
  ~ChromeKeymaster();

  bool Begin(scoped_refptr<net::X509Certificate> cert,
             const std::vector<mojom::KeyParamPtr>& params,
             uint64_t* operation_handle);

  bool Update(uint64_t operation_handle,
              const std::vector<uint8_t>& data,
              int32_t* input_consumed);

  bool Abort(uint64_t operation_handle);

  bool Finish(uint64_t operation_handle, std::vector<uint8_t>* signature);

  std::vector<mojom::KeyParamPtr> GetKeyCharacteristics(
      const scoped_refptr<net::X509Certificate>& cert);

 private:
  std::map<uint64_t, struct OperationInfo> operation_table_;
  std::set<int> slot_ids_;

  DISALLOW_COPY_AND_ASSIGN(ChromeKeymaster);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ENTERPRISE_CHROME_KEYMASTER_H_
