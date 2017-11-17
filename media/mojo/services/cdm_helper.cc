// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/cdm_helper.h"

#include "base/logging.h"
#include "media/base/cdm_context.h"
#include "media/mojo/services/mojo_cdm_service_context.h"

namespace media {

CdmHelper::CdmHelper() {}
CdmHelper::~CdmHelper() {}

bool CdmHelper::Configure(MojoCdmServiceContext* mojo_cdm_service_context,
                          int32_t cdm_id) {
  if (!mojo_cdm_service_context) {
    DVLOG(1) << "CDM service context not available.";
    return false;
  }

  cdm = mojo_cdm_service_context->GetCdm(cdm_id);
  if (!cdm) {
    DVLOG(1) << "CDM not found for CDM id: " << cdm_id;
    return false;
  }

  cdm_context = cdm->GetCdmContext();
  if (!cdm_context) {
    DVLOG(1) << "CDM context not available for CDM id: " << cdm_id;
    return false;
  }

  return true;
}

}  // namespace media
