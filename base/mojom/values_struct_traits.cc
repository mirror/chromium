// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mojom/values_struct_traits.h"

#include <memory>
#include <utility>

#include "base/strings/string_piece.h"

namespace mojo {

bool StructTraits<base::mojom::DictionaryValueDataView, base::Value>::Read(
    base::mojom::DictionaryValueDataView data,
    base::Value* value_out) {
  mojo::MapDataView<mojo::StringDataView, base::mojom::ValueDataView> view;
  data.GetStorageDataView(&view);
  std::vector<base::Value::DictStorage::value_type> dict_storage;
  dict_storage.reserve(view.size());
  for (size_t i = 0; i < view.size(); ++i) {
    base::StringPiece key;
    auto value = std::make_unique<base::Value>();
    if (!view.keys().Read(i, &key) || !view.values().Read(i, value.get()))
      return false;
    dict_storage.emplace_back(key.as_string(), std::move(value));
  }
  *value_out = base::Value(base::Value::DictStorage(std::move(dict_storage),
                                                    base::KEEP_LAST_OF_DUPES));
  return true;
}

bool StructTraits<base::mojom::ListValueDataView, base::Value>::Read(
    base::mojom::ListValueDataView data,
    base::Value* value_out) {
  mojo::ArrayDataView<base::mojom::ValueDataView> view;
  data.GetStorageDataView(&view);
  base::Value::ListStorage list_storage(view.size());
  for (size_t i = 0; i < view.size(); ++i) {
    if (!view.Read(i, &list_storage[i]))
      return false;
  }
  *value_out = base::Value(std::move(list_storage));
  return true;
}

bool UnionTraits<base::mojom::ValueDataView, base::Value>::Read(
    base::mojom::ValueDataView data,
    base::Value* value_out) {
  switch (data.tag()) {
    case base::mojom::ValueDataView::Tag::NULL_VALUE: {
      *value_out = base::Value();
      return true;
    }
    case base::mojom::ValueDataView::Tag::BOOL_VALUE: {
      *value_out = base::Value(data.bool_value());
      return true;
    }
    case base::mojom::ValueDataView::Tag::INT_VALUE: {
      *value_out = base::Value(data.int_value());
      return true;
    }
    case base::mojom::ValueDataView::Tag::DOUBLE_VALUE: {
      *value_out = base::Value(data.double_value());
      return true;
    }
    case base::mojom::ValueDataView::Tag::STRING_VALUE: {
      base::StringPiece string_piece;
      if (!data.ReadStringValue(&string_piece))
        return false;
      *value_out = base::Value(string_piece);
      return true;
    }
    case base::mojom::ValueDataView::Tag::BINARY_VALUE: {
      mojo::ArrayDataView<uint8_t> binary_data_view;
      data.GetBinaryValueDataView(&binary_data_view);
      const char* data_pointer =
          reinterpret_cast<const char*>(binary_data_view.data());
      base::Value::BlobStorage blob_storage(
          data_pointer, data_pointer + binary_data_view.size());
      *value_out = base::Value(std::move(blob_storage));
      return true;
    }
    case base::mojom::ValueDataView::Tag::DICTIONARY_VALUE: {
      return data.ReadDictionaryValue(value_out);
    }
    case base::mojom::ValueDataView::Tag::LIST_VALUE: {
      return data.ReadListValue(value_out);
    }
  }
  return false;
}

}  // namespace mojo
