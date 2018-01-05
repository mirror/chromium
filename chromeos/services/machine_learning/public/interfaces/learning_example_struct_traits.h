// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef CHROMEOS_SERVICES_MACHINE_LEARNING_PUBLIC_INTERFACES_LEARNING_EXAMPLE_STRUCT_TRAITS_H_
#define CHROMEOS_SERVICES_MACHINE_LEARNING_PUBLIC_INTERFACES_LEARNING_EXAMPLE_STRUCT_TRAITS_H_

#include "chromeos/services/machine_learning/public/interfaces/learning_example.mojom.h"
#include "components/learning/learning_example.h"

namespace mojo {

template <>
class StructTraits<chromeos.mojom.LearningExampleView,
                   learning::LearningExample> {
 public:
  static bool Read(chromeos::mojom::LearningExampleDataView data,
                   learning::LearningExample* out) {
    *out = data;
    return true;
  }
};

}  // namespace mojo

#endif  // CHROMEOS_SERVICES_MACHINE_LEARNING_PUBLIC_INTERFACES_LEARNING_EXAMPLE_STRUCT_TRAITS_H_