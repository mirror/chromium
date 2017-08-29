// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MOJOM_VALUES_STRUCT_TRAITS_H_
#define BASE_MOJOM_VALUES_STRUCT_TRAITS_H_

#include <vector>

#include "base/mojom/values.mojom.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/map_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "mojo/public/cpp/bindings/union_traits.h"

namespace mojo {

template <>
struct MapTraits<base::Value> {
  using Key = std::string;
  using Value = base::Value;
  using Iterator = base::Value::const_dict_iterator_proxy::const_iterator;

  static size_t GetSize(const base::Value& input) {
    DCHECK(input.is_dict());
    return static_cast<const base::DictionaryValue&>(input).size();
  }

  static Iterator GetBegin(const base::Value& input) {
    DCHECK(input.is_dict());
    return input.DictItems().cbegin();
  }

  static void AdvanceIterator(Iterator& iterator) { ++iterator; }

  static const Key& GetKey(Iterator& iterator) { return iterator->first; }

  static const Value& GetValue(Iterator& iterator) { return iterator->second; }
};

template <>
struct StructTraits<base::mojom::DictionaryValueDataView, base::Value> {
  static const base::Value& storage(const base::Value& value) {
    DCHECK(value.is_dict());
    return value;
  }

  static bool Read(base::mojom::DictionaryValueDataView data,
                   base::Value* value);
};

template <>
struct StructTraits<base::mojom::ListValueDataView, base::Value> {
  static const base::Value::ListStorage& storage(const base::Value& value) {
    DCHECK(value.is_list());
    return value.GetList();
  }

  static bool Read(base::mojom::ListValueDataView data, base::Value* value);
};

template <>
struct UnionTraits<base::mojom::ValueDataView, base::Value> {
  static base::mojom::ValueDataView::Tag GetTag(const base::Value& data) {
    switch (data.type()) {
      case base::Value::Type::NONE:
        return base::mojom::ValueDataView::Tag::NULL_VALUE;
      case base::Value::Type::BOOLEAN:
        return base::mojom::ValueDataView::Tag::BOOL_VALUE;
      case base::Value::Type::INTEGER:
        return base::mojom::ValueDataView::Tag::INT_VALUE;
      case base::Value::Type::DOUBLE:
        return base::mojom::ValueDataView::Tag::DOUBLE_VALUE;
      case base::Value::Type::STRING:
        return base::mojom::ValueDataView::Tag::STRING_VALUE;
      case base::Value::Type::BINARY:
        return base::mojom::ValueDataView::Tag::BINARY_VALUE;
      case base::Value::Type::DICTIONARY:
        return base::mojom::ValueDataView::Tag::DICTIONARY_VALUE;
      case base::Value::Type::LIST:
        return base::mojom::ValueDataView::Tag::LIST_VALUE;
    }
    NOTREACHED();
    return base::mojom::ValueDataView::Tag::NULL_VALUE;
  }

  static uint8_t null_value(const base::Value& value) { return 0; }

  static bool bool_value(const base::Value& value) { return value.GetBool(); }

  static int32_t int_value(const base::Value& value) { return value.GetInt(); }

  static double double_value(const base::Value& value) {
    return value.GetDouble();
  }

  static base::StringPiece string_value(const base::Value& value) {
    return value.GetString();
  }

  static base::span<const uint8_t> binary_value(const base::Value& value) {
    // TODO(dcheng): Change base::Value::BlobStorage to uint8_t.
    return base::make_span(
        reinterpret_cast<const uint8_t*>(value.GetBlob().data()),
        value.GetBlob().size());
  }

  static const base::Value& list_value(const base::Value& value) {
    DCHECK(value.is_list());
    return value;
  }
  static const base::Value& dictionary_value(const base::Value& value) {
    DCHECK(value.is_dict());
    return value;
  }

  static bool Read(base::mojom::ValueDataView view, base::Value* value_out);
};

}  // namespace mojo

#endif  // BASE_MOJOM_VALUES_STRUCT_TRAITS_H_
