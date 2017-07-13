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
struct MapTraits<base::DictionaryValue> {
  using Key = std::string;
  using Value = base::Value;
  using Iterator = base::DictionaryValue::Iterator;

  static size_t GetSize(const base::DictionaryValue& input) {
    return input.size();
  }

  static Iterator GetBegin(const base::DictionaryValue& input) {
    return Iterator(input);
  }

  static void AdvanceIterator(Iterator& iterator) { iterator.Advance(); }

  static const Key& GetKey(Iterator& iterator) { return iterator.key(); }

  static const Value& GetValue(Iterator& iterator) { return iterator.value(); }
};

template <>
struct StructTraits<base::mojom::InternalDictValueDoNotUseDataView,
                    base::DictionaryValue> {
  static const base::DictionaryValue& storage(
      const base::DictionaryValue& value) {
    return value;
  }

  static bool Read(base::mojom::InternalDictValueDoNotUseDataView data,
                   base::Value* value);
};

template <>
struct UnionTraits<base::mojom::ValueDataView, base::Value> {
  static base::mojom::ValueDataView::Tag GetTag(const base::Value& data) {
    switch (data.type()) {
      case base::Value::Type::NONE:
        return base::mojom::ValueDataView::Tag::NULL_VALUE;
        break;
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

  static mojo::ConstCArray<uint8_t> binary_value(const base::Value& value) {
    // TODO(dcheng): Change base::Value::BlobStorage to uint8_t.
    return mojo::ConstCArray<uint8_t>(
        value.GetBlob().size(),
        reinterpret_cast<const uint8_t*>(value.GetBlob().data()));
  }

  static const std::vector<base::Value>& list_value(const base::Value& value) {
    return value.GetList();
  }
  static const base::DictionaryValue& dictionary_value(
      const base::Value& value) {
    const base::DictionaryValue* dictionary_value = nullptr;
    if (!value.GetAsDictionary(&dictionary_value))
      NOTREACHED();
    return *dictionary_value;
  }

  static bool Read(base::mojom::ValueDataView view, base::Value* value_out);
};

}  // namespace mojo

#endif  // BASE_MOJOM_VALUES_STRUCT_TRAITS_H_
