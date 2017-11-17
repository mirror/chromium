// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_uma.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/strings/string_number_conversions.h"

namespace device {

namespace {

int CombineVendorAndProductIds(const base::StringPiece& vendor_id,
                               const base::StringPiece& product_id) {
  int vendor_id_int, product_id_int;
  if (!base::StringToInt(vendor_id, &vendor_id_int))
    vendor_id_int = 0;
  if (!base::StringToInt(product_id, &product_id_int))
    product_id_int = 0;
  uint32_t data = ((vendor_id_int & 0xffff) << 16) + product_id_int & 0xffff;

  // Strip off the sign bit because UMA doesn't support negative values but
  // takes a signed int. This removes the high bit from the vendor ID, which is
  // typically not needed to uniquely identify a device.
  return static_cast<int>(data & 0x7fffffff);
}

}  // namespace

void RecordConnectedGamepadInfo(const base::StringPiece& vendor_id,
                                const base::StringPiece& product_id,
                                bool has_standard_mapping) {
  if (has_standard_mapping) {
    UMA_HISTOGRAM_SPARSE_SLOWLY(
        "Gamepad.Web.StandardMapping.Devices",
        CombineVendorAndProductIds(vendor_id, product_id));
  } else {
    UMA_HISTOGRAM_SPARSE_SLOWLY(
        "Gamepad.Web.NoMapping.Devices",
        CombineVendorAndProductIds(vendor_id, product_id));
  }
}

}  // namespace device
