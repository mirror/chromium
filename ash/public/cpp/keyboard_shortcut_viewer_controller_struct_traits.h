// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_STRUCT_TRAITS_H_
#define ASH_PUBLIC_CPP_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_STRUCT_TRAITS_H_

#include <vector>

#include "ash/public/interfaces/keyboard_shortcut_viewer_controller.mojom.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::ShortcutReplacementType,
                  keyboard_shortcut_viewer::ShortcutReplacementType> {
  static ash::mojom::ShortcutReplacementType ToMojom(
      keyboard_shortcut_viewer::ShortcutReplacementType input) {
    switch (input) {
      case keyboard_shortcut_viewer::ShortcutReplacementType::MODIFIER:
        return ash::mojom::ShortcutReplacementType::MODIFIER;
      case keyboard_shortcut_viewer::ShortcutReplacementType::VKEY:
        return ash::mojom::ShortcutReplacementType::VKEY;
      case keyboard_shortcut_viewer::ShortcutReplacementType::ICON:
        return ash::mojom::ShortcutReplacementType::ICON;
    }
    NOTREACHED();
    return ash::mojom::ShortcutReplacementType::MODIFIER;
  }

  static bool FromMojom(
      ash::mojom::ShortcutReplacementType input,
      keyboard_shortcut_viewer::ShortcutReplacementType* out) {
    switch (input) {
      case ash::mojom::ShortcutReplacementType::MODIFIER:
        *out = keyboard_shortcut_viewer::ShortcutReplacementType::MODIFIER;
        return true;
      case ash::mojom::ShortcutReplacementType::VKEY:
        *out = keyboard_shortcut_viewer::ShortcutReplacementType::VKEY;
        return true;
      case ash::mojom::ShortcutReplacementType::ICON:
        *out = keyboard_shortcut_viewer::ShortcutReplacementType::ICON;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<ash::mojom::ShortcutReplacementInfoDataView,
                    keyboard_shortcut_viewer::ShortcutReplacementInfo> {
  static int32_t replacement_id(
      const keyboard_shortcut_viewer::ShortcutReplacementInfo& i) {
    return i.replacement_id;
  }
  static const keyboard_shortcut_viewer::ShortcutReplacementType&
  replacement_type(const keyboard_shortcut_viewer::ShortcutReplacementInfo& i) {
    return i.replacement_type;
  }
  static bool Read(ash::mojom::ShortcutReplacementInfoDataView data,
                   keyboard_shortcut_viewer::ShortcutReplacementInfo* out);
};

template <>
struct ASH_PUBLIC_EXPORT
    StructTraits<ash::mojom::KeyboardShortcutItemInfoDataView,
                 keyboard_shortcut_viewer::KeyboardShortcutItemInfo> {
  static const std::vector<int>& categories(
      const keyboard_shortcut_viewer::KeyboardShortcutItemInfo& i) {
    return i.categories;
  }
  static int32_t description(
      const keyboard_shortcut_viewer::KeyboardShortcutItemInfo& i) {
    return i.description;
  }
  static int32_t shortcut(
      const keyboard_shortcut_viewer::KeyboardShortcutItemInfo& i) {
    return i.shortcut;
  }
  static const std::vector<keyboard_shortcut_viewer::ShortcutReplacementInfo>&
  shortcut_replacements_info(
      const keyboard_shortcut_viewer::KeyboardShortcutItemInfo& i) {
    return i.shortcut_replacements_info;
  }
  static bool Read(ash::mojom::KeyboardShortcutItemInfoDataView data,
                   keyboard_shortcut_viewer::KeyboardShortcutItemInfo* out);
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_STRUCT_TRAITS_H_
