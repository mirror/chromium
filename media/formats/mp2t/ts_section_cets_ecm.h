// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_MP2T_TS_SECTION_CETS_ECM_H_
#define MEDIA_FORMATS_MP2T_TS_SECTION_CETS_ECM_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "media/formats/mp2t/ts_section.h"

namespace media {

namespace mp2t {

class TsSectionCetsEcm : public TsSection {
 public:
  using RegisterNewKeyIdAndIvCb =
      base::RepeatingCallback<void(const std::string& key_id,
                                   const std::string& iv)>;
  explicit TsSectionCetsEcm(
      const RegisterNewKeyIdAndIvCb& register_new_key_id_and_iv_cb);
  ~TsSectionCetsEcm() override;

  // TsSection implementation.
  bool Parse(bool payload_unit_start_indicator,
             const uint8_t* buf,
             int size) override;
  void Flush() override;
  void Reset() override;

 private:
  RegisterNewKeyIdAndIvCb register_new_key_id_and_iv_cb_;

  DISALLOW_COPY_AND_ASSIGN(TsSectionCetsEcm);
};

}  // namespace mp2t
}  // namespace media

#endif
