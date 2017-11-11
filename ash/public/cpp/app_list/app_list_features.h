// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_
#define ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_

#include <string>

#include "ash/public/cpp/app_list/app_list_export.h"
#include "ash/public/cpp/ash_public_export.h"

namespace base {
struct Feature;
}

namespace app_list {
namespace features {

// Please keep these features sorted.

// Enables the answer card in the app list.
ASH_PUBLIC_EXPORT APP_LIST_EXPORT extern const base::Feature kEnableAnswerCard;

// Enables background blur for the app list, lock screen, and tab switcher, also
// enables the AppsGridView mask layer. In this mode, slower devices may have
// choppier app list animations. crbug.com/765292.
ASH_PUBLIC_EXPORT APP_LIST_EXPORT extern const base::Feature
    kEnableBackgroundBlur;

// Enables the Play Store app search.
ASH_PUBLIC_EXPORT APP_LIST_EXPORT extern const base::Feature
    kEnablePlayStoreAppSearch;

// Enables the app list focus. In this mode, many views become focusable. Focus
// transition are handled by FocusManager and accessibility focus transition can
// be triggered properly on search+arrow key as standard.
// TODO(weidongg/766807) Remove this flag when the related changes become
// stable.
ASH_PUBLIC_EXPORT APP_LIST_EXPORT extern const base::Feature
    kEnableAppListFocus;

bool ASH_PUBLIC_EXPORT APP_LIST_EXPORT IsAnswerCardEnabled();
bool ASH_PUBLIC_EXPORT APP_LIST_EXPORT IsBackgroundBlurEnabled();
bool ASH_PUBLIC_EXPORT APP_LIST_EXPORT IsFullscreenAppListEnabled();
bool ASH_PUBLIC_EXPORT APP_LIST_EXPORT IsPlayStoreAppSearchEnabled();
bool ASH_PUBLIC_EXPORT APP_LIST_EXPORT IsAppListFocusEnabled();
std::string ASH_PUBLIC_EXPORT APP_LIST_EXPORT AnswerServerUrl();
std::string ASH_PUBLIC_EXPORT APP_LIST_EXPORT AnswerServerQuerySuffix();

}  // namespace features
}  // namespace app_list

#endif  // ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_
