// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/core/v8/ReferrerScriptInfo.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

enum HostDefinedOptionsIndex : size_t { kURL, kLength };

}  // namespace

ReferrerScriptInfo ReferrerScriptInfo::FromV8HostDefinedOptions(
    v8::Local<v8::PrimitiveArray> host_defined_options) {
  if (host_defined_options.IsEmpty()) {
    String empty_url;
    String empty_nonce;
    return ReferrerScriptInfo(empty_url,
                              WebURLRequest::kFetchCredentialsModeOmit,
                              empty_nonce, kNotParserInserted);
  }

  v8::Local<v8::Primitive> url_value = host_defined_options->Get(0);
  String url =
      ToCoreStringWithNullCheck(v8::Local<v8::String>::Cast(url_value));

  String empty_nonce;
  // TODO(kouhei)
  return ReferrerScriptInfo(url, WebURLRequest::kFetchCredentialsModeOmit,
                            empty_nonce, kNotParserInserted);
}

v8::Local<v8::PrimitiveArray> ReferrerScriptInfo::ToV8HostDefinedOptions(
    v8::Isolate* isolate) const {
  v8::Local<v8::PrimitiveArray> host_defined_options =
      v8::PrimitiveArray::New(isolate, HostDefinedOptionsIndex::kLength);

  v8::Local<v8::Primitive> url_value = V8String(isolate, url_);
  host_defined_options->Set(HostDefinedOptionsIndex::kURL, url_value);

  return host_defined_options;
}

}  // namespace blink
