// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_PUBLIC_INTERFACES_AUTOFILL_TYPES_STRUCT_TRAITS_H_
#define COMPONENTS_AUTOFILL_CONTENT_PUBLIC_INTERFACES_AUTOFILL_TYPES_STRUCT_TRAITS_H_

#include "base/strings/string16.h"
#include "components/autofill/content/public/interfaces/autofill_types.mojom.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct EnumTraits<autofill::mojom::CheckStatus,
                  autofill::FormFieldData::CheckStatus> {
  static autofill::mojom::CheckStatus ToMojom(
      autofill::FormFieldData::CheckStatus input);
  static bool FromMojom(autofill::mojom::CheckStatus input,
                        autofill::FormFieldData::CheckStatus* output);
};

template <>
struct EnumTraits<autofill::mojom::RoleAttribute,
                  autofill::FormFieldData::RoleAttribute> {
  static autofill::mojom::RoleAttribute ToMojom(
      autofill::FormFieldData::RoleAttribute input);
  static bool FromMojom(autofill::mojom::RoleAttribute input,
                        autofill::FormFieldData::RoleAttribute* output);
};

template <>
struct EnumTraits<autofill::mojom::TextDirection, base::i18n::TextDirection> {
  static autofill::mojom::TextDirection ToMojom(
      base::i18n::TextDirection input);
  static bool FromMojom(autofill::mojom::TextDirection input,
                        base::i18n::TextDirection* output);
};

template <>
struct StructTraits<autofill::mojom::FormFieldData, autofill::FormFieldData> {
  static const base::string16& label(const autofill::FormFieldData& r) {
    return r.label;
  }

  static const base::string16& name(const autofill::FormFieldData& r) {
    return r.name;
  }

  static const base::string16& value(const autofill::FormFieldData& r) {
    return r.value;
  }

  static const std::string& form_control_type(
      const autofill::FormFieldData& r) {
    return r.form_control_type;
  }

  static const std::string& autocomplete_attribute(
      const autofill::FormFieldData& r) {
    return r.autocomplete_attribute;
  }

  static const base::string16& placeholder(const autofill::FormFieldData& r) {
    return r.placeholder;
  }

  static uint64_t max_length(const autofill::FormFieldData& r) {
    return r.max_length;
  }

  static bool is_autofilled(const autofill::FormFieldData& r) {
    return r.is_autofilled;
  }

  static autofill::FormFieldData::CheckStatus check_status(
      const autofill::FormFieldData& r) {
    return r.check_status;
  }

  static bool is_focusable(const autofill::FormFieldData& r) {
    return r.is_focusable;
  }

  static bool should_autocomplete(const autofill::FormFieldData& r) {
    return r.should_autocomplete;
  }

  static autofill::FormFieldData::RoleAttribute role(
      const autofill::FormFieldData& r) {
    return r.role;
  }

  static base::i18n::TextDirection text_direction(
      const autofill::FormFieldData& r) {
    return r.text_direction;
  }

  static const std::vector<base::string16>& option_values(
      const autofill::FormFieldData& r) {
    return r.option_values;
  }

  static const std::vector<base::string16>& option_contents(
      const autofill::FormFieldData& r) {
    return r.option_contents;
  }

  static bool Read(autofill::mojom::FormFieldDataDataView data,
                   autofill::FormFieldData* out);
};

template <>
struct StructTraits<autofill::mojom::FormData, autofill::FormData> {
  static const base::string16& name(const autofill::FormData& r) {
    return r.name;
  }

  static const GURL& origin(const autofill::FormData& r) { return r.origin; }

  static const GURL& action(const autofill::FormData& r) { return r.action; }

  static bool is_form_tag(const autofill::FormData& r) { return r.is_form_tag; }

  static bool is_formless_checkout(const autofill::FormData& r) {
    return r.is_formless_checkout;
  }

  static const std::vector<autofill::FormFieldData>& fields(
      const autofill::FormData& r) {
    return r.fields;
  }

  static bool Read(autofill::mojom::FormDataDataView data,
                   autofill::FormData* out);
};

}  // namespace mojo

#endif  // COMPONENTS_AUTOFILL_CONTENT_PUBLIC_INTERFACES_AUTOFILL_TYPES_STRUCT_TRAITS_H_
