// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_APP_LIST_STRUCT_TRAITS_H_
#define ASH_PUBLIC_CPP_APP_LIST_APP_LIST_STRUCT_TRAITS_H_

#include "ash/public/cpp/app_list/app_list_types.h"
#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/interfaces/app_list.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::AppListState, ash::AppListState> {
  static ash::mojom::AppListState ToMojom(ash::AppListState input) {
    switch (input) {
      case ash::AppListState::kStateApps:
        return ash::mojom::AppListState::STATE_APPS;
      case ash::AppListState::kStateSearchResults:
        return ash::mojom::AppListState::STATE_SEARCH_RESULTS;
      case ash::AppListState::kStateStart:
        return ash::mojom::AppListState::STATE_START;
      case ash::AppListState::kStateCustomLauncherPageDeprecated:
      case ash::AppListState::kInvalidState:
        break;
    }
    NOTREACHED();
    return ash::mojom::AppListState::STATE_APPS;
  }

  static bool FromMojom(ash::mojom::AppListState input,
                        ash::AppListState* out) {
    switch (input) {
      case ash::mojom::AppListState::STATE_APPS:
        *out = ash::AppListState::kStateApps;
        return true;
      case ash::mojom::AppListState::STATE_SEARCH_RESULTS:
        *out = ash::AppListState::kStateSearchResults;
        return true;
      case ash::mojom::AppListState::STATE_START:
        *out = ash::AppListState::kStateStart;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_APP_LIST_APP_LIST_STRUCT_TRAITS_H_
