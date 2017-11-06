// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_STRUCT_TRAITS_H_
#define ASH_PUBLIC_CPP_APP_LIST_STRUCT_TRAITS_H_

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/interfaces/app_list.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::AppListModelStatus,
                  app_list::AppListModel::Status> {
  static ash::mojom::AppListModelStatus ToMojom(
      app_list::AppListModel::Status input) {
    switch (input) {
      case app_list::AppListModel::Status::STATUS_NORMAL:
        return ash::mojom::AppListModelStatus::STATUS_NORMAL;
      case app_list::AppListModel::Status::STATUS_SYNCING:
        return ash::mojom::AppListModelStatus::STATUS_SYNCING;
    }
    NOTREACHED();
    return ash::mojom::AppListModelStatus::STATUS_NORMAL;
  }

  static bool FromMojom(ash::mojom::AppListModelStatus input,
                        app_list::AppListModel::Status* out) {
    switch (input) {
      case ash::mojom::AppListModelStatus::STATUS_NORMAL:
        *out = app_list::AppListModel::Status::STATUS_NORMAL;
        return true;
      case ash::mojom::AppListModelStatus::STATUS_SYNCING:
        *out = app_list::AppListModel::Status::STATUS_SYNCING;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<ash::mojom::AppListModelState,
                  app_list::AppListModel::State> {
  static ash::mojom::AppListModelState ToMojom(
      app_list::AppListModel::State input) {
    switch (input) {
      case app_list::AppListModel::State::STATE_APPS:
        return ash::mojom::AppListModelState::STATE_APPS;
      case app_list::AppListModel::State::STATE_SEARCH_RESULTS:
        return ash::mojom::AppListModelState::STATE_SEARCH_RESULTS;
      case app_list::AppListModel::State::STATE_START:
        return ash::mojom::AppListModelState::STATE_START;
      case app_list::AppListModel::State::STATE_CUSTOM_LAUNCHER_PAGE:
        return ash::mojom::AppListModelState::STATE_CUSTOM_LAUNCHER_PAGE;
      case app_list::AppListModel::State::INVALID_STATE:
        return ash::mojom::AppListModelState::INVALID_STATE;
    }
    NOTREACHED();
    return ash::mojom::AppListModelState::INVALID_STATE;
  }

  static bool FromMojom(ash::mojom::AppListModelState input,
                        app_list::AppListModel::State* out) {
    switch (input) {
      case ash::mojom::AppListModelState::STATE_APPS:
        *out = app_list::AppListModel::State::STATE_APPS;
        return true;
      case ash::mojom::AppListModelState::STATE_SEARCH_RESULTS:
        *out = app_list::AppListModel::State::STATE_SEARCH_RESULTS;
        return true;
      case ash::mojom::AppListModelState::STATE_START:
        *out = app_list::AppListModel::State::STATE_START;
        return true;
      case ash::mojom::AppListModelState::STATE_CUSTOM_LAUNCHER_PAGE:
        *out = app_list::AppListModel::State::STATE_CUSTOM_LAUNCHER_PAGE;
        return true;
      case ash::mojom::AppListModelState::INVALID_STATE:
        *out = app_list::AppListModel::State::INVALID_STATE;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_APP_LIST_STRUCT_TRAITS_H_
