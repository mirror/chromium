// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/BigStringStructTraits.h"

#include <cstring>

#include "base/containers/span.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/base/big_buffer_struct_traits.h"
#include "platform/wtf/text/StringUTF8Adaptor.h"

namespace mojo {

namespace {

struct UTF8AdaptorInfo {
  explicit UTF8AdaptorInfo(const WTF::String& input) : utf8_adaptor(input) {
#if DCHECK_IS_ON()
    original_size_in_bytes = input.CharactersSizeInBytes();
#endif
  }

  ~UTF8AdaptorInfo() {}

  WTF::StringUTF8Adaptor utf8_adaptor;

#if DCHECK_IS_ON()
  // For sanity check only.
  size_t original_size_in_bytes;
#endif
};

UTF8AdaptorInfo* ToAdaptor(const WTF::String& input, void* context) {
  UTF8AdaptorInfo* adaptor = static_cast<UTF8AdaptorInfo*>(context);

#if DCHECK_IS_ON()
  DCHECK_EQ(adaptor->original_size_in_bytes, input.CharactersSizeInBytes());
#endif
  return adaptor;
}

}  // namespace

// static
void* StructTraits<common::mojom::BigStringDataView, WTF::String>::SetUpContext(
    const WTF::String& input) {
  return new UTF8AdaptorInfo(input);
}

// static
void StructTraits<common::mojom::BigStringDataView,
                  WTF::String>::TearDownContext(const WTF::String& input,
                                                void* context) {
  delete ToAdaptor(input, context);
}

// static
mojo_base::BigBuffer StructTraits<common::mojom::BigStringDataView,
                                  WTF::String>::data(const WTF::String& input,
                                                     void* context) {
  UTF8AdaptorInfo* adaptor = ToAdaptor(input, context);
  return mojo_base::BigBuffer(base::make_span(
      reinterpret_cast<const uint8_t*>(adaptor->utf8_adaptor.Data()),
      adaptor->utf8_adaptor.length() * sizeof(char)));
}

// static
bool StructTraits<common::mojom::BigStringDataView, WTF::String>::Read(
    common::mojom::BigStringDataView data,
    WTF::String* out) {
  mojo_base::BigBuffer buffer;
  if (!data.ReadData(&buffer))
    return false;
  size_t size = buffer.size();
  if (size % sizeof(char))
    return false;

  // An empty |mojo_base::BigBuffer| may have a null |data()| if empty.
  if (!size) {
    *out = g_empty_string;
  } else {
    WTF::String result = WTF::String::FromUTF8(
        reinterpret_cast<const char*>(buffer.data()), size / sizeof(char));
    out->swap(result);
  }

  return true;
}

}  // namespace mojo
