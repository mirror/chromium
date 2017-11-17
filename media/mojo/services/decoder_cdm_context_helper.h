// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_CDM_HELPER_H_
#define MEDIA_MOJO_SERVICES_CDM_HELPER_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "media/base/content_decryption_module.h"

namespace media {

class CdmContext;
class ContentDecryptionModule;
class MojoCdmServiceContext;

// Helper class for Cdm setup for mojo audio/video decoders.
struct DecoderCdmContextHelper {
  DecoderCdmContextHelper();
  ~DecoderCdmContextHelper();

  CdmContext* cdm_context = nullptr;
  scoped_refptr<ContentDecryptionModule> cdm;

  // Set up |cdm_context| and |cdm|.
  // Returns true on success.
  bool Configure(MojoCdmServiceContext* mojo_cdm_service_context,
                 int32_t cdm_id);

 private:
  DISALLOW_COPY_AND_ASSIGN(DecoderCdmContextHelper);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_CDM_HELPER_H_
