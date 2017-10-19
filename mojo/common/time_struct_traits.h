// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_COMMON_TIME_STRUCT_TRAITS_H_
#define MOJO_COMMON_TIME_STRUCT_TRAITS_H_

#include "base/time/time.h"
#include "mojo/common/time.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<common::mojom::TimeDataView, base::Time> {
  static base::TimeDelta origin_offset(const base::Time& time) {
    return time.since_origin();
  }

  static bool Read(common::mojom::TimeDataView data, base::Time* time) {
    base::TimeDelta origin_offset;
    if (!data.ReadOriginOffset(&origin_offset))
      return false;
    *time = base::Time() + origin_offset;
    return true;
  }
};

template <>
struct StructTraits<common::mojom::TimeDeltaDataView, base::TimeDelta> {
  static int64_t microseconds(const base::TimeDelta& delta) {
    return delta.InMicroseconds();
  }

  static bool Read(common::mojom::TimeDeltaDataView data,
                   base::TimeDelta* delta) {
    *delta = base::TimeDelta::FromMicroseconds(data.microseconds());
    return true;
  }
};

template <>
struct StructTraits<common::mojom::TimeTicksDataView, base::TimeTicks> {
  static base::TimeDelta origin_offset(const base::TimeTicks& time) {
    return time.since_origin();
  }

  static bool Read(common::mojom::TimeTicksDataView data,
                   base::TimeTicks* time) {
    base::TimeDelta origin_offset;
    if (!data.ReadOriginOffset(&origin_offset))
      return false;
    *time = base::TimeTicks() + origin_offset;
    return true;
  }
};

}  // namespace mojo

#endif  // MOJO_COMMON_TIME_STRUCT_TRAITS_H_
