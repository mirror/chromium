// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_CDM_OUTPUT_PROTECTION_H_
#define MEDIA_BASE_CDM_OUTPUT_PROTECTION_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT CdmOutputProtection {
 public:
  CdmOutputProtection() = default;
  virtual ~CdmOutputProtection();

  using QueryStatusCB = base::OnceCallback<
      void(bool success, uint32_t link_mask, uint32_t protection_mask)>;
  using EnableProtectionCB = base::OnceCallback<void(bool success)>;

  // Queries link status and protection status. Clients need to query status
  // periodically in order to detect changes. |callback| will be called with
  // the following values:
  // - success: Whether the query succeeded. If false, values of |link_mask|
  //   and |protection_mask| should be ignored.
  // - link_mask: The type of connected output links, which is a bit-mask of
  //   the LinkType values.
  // - protection_mask: The type of enabled protections, which is a bit-mask
  //   of the ProtectionType values.
  virtual void QueryStatus(QueryStatusCB callback) = 0;

  // Sets desired protection methods. |desired_protection_mask| is a bit-mask
  // of ProtectionType values. Calls |callback| when the request has been made.
  // Users should call QueryStatus() to verify that it was actually applied.
  // Protections will be disabled if no longer desired by all instances.
  // |callback| will be called with the following value:
  // - success: True when the protection request has been made. This may be
  //   before the protection have actually been applied. False if it failed
  //   to make the protection request, and in this case there is no need to
  //   call QueryStatus().
  virtual void EnableProtection(uint32_t desired_protection_mask,
                                EnableProtectionCB callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CdmOutputProtection);
};

}  // namespace media

#endif  // MEDIA_BASE_CDM_OUTPUT_PROTECTION_H_
