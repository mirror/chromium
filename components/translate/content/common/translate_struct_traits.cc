// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/content/common/translate_struct_traits.h"

#include "ipc/ipc_message_utils.h"
#include "url/mojo/url_gurl_struct_traits.h"

using translate::LanguageDetectionDetails;
using translate::TranslateErrors;

namespace mojo {

translate::mojom::TranslateError
EnumTraits<translate::mojom::TranslateError, TranslateErrors::Type>::ToMojom(
    TranslateErrors::Type input) {
  switch (input) {
    case TranslateErrors::Type::NONE:
      return translate::mojom::TranslateError::NONE;
    case TranslateErrors::Type::NETWORK:
      return translate::mojom::TranslateError::NETWORK;
    case TranslateErrors::Type::INITIALIZATION_ERROR:
      return translate::mojom::TranslateError::INITIALIZATION_ERROR;
    case TranslateErrors::Type::UNKNOWN_LANGUAGE:
      return translate::mojom::TranslateError::UNKNOWN_LANGUAGE;
    case TranslateErrors::Type::UNSUPPORTED_LANGUAGE:
      return translate::mojom::TranslateError::UNSUPPORTED_LANGUAGE;
    case TranslateErrors::Type::IDENTICAL_LANGUAGES:
      return translate::mojom::TranslateError::IDENTICAL_LANGUAGES;
    case TranslateErrors::Type::TRANSLATION_ERROR:
      return translate::mojom::TranslateError::TRANSLATION_ERROR;
    case TranslateErrors::Type::TRANSLATION_TIMEOUT:
      return translate::mojom::TranslateError::TRANSLATION_TIMEOUT;
    case TranslateErrors::Type::UNEXPECTED_SCRIPT_ERROR:
      return translate::mojom::TranslateError::UNEXPECTED_SCRIPT_ERROR;
    case TranslateErrors::Type::BAD_ORIGIN:
      return translate::mojom::TranslateError::BAD_ORIGIN;
    case TranslateErrors::Type::SCRIPT_LOAD_ERROR:
      return translate::mojom::TranslateError::SCRIPT_LOAD_ERROR;
    case TranslateErrors::Type::TRANSLATE_ERROR_MAX:
      return translate::mojom::TranslateError::TRANSLATE_ERROR_MAX;
  }

  NOTREACHED();
  return translate::mojom::TranslateError::NONE;
}

bool EnumTraits<translate::mojom::TranslateError, TranslateErrors::Type>::
    FromMojom(translate::mojom::TranslateError input,
              TranslateErrors::Type* output) {
  switch (input) {
    case translate::mojom::TranslateError::NONE:
      *output = TranslateErrors::Type::NONE;
      return true;
    case translate::mojom::TranslateError::NETWORK:
      *output = TranslateErrors::Type::NETWORK;
      return true;
    case translate::mojom::TranslateError::INITIALIZATION_ERROR:
      *output = TranslateErrors::Type::INITIALIZATION_ERROR;
      return true;
    case translate::mojom::TranslateError::UNKNOWN_LANGUAGE:
      *output = TranslateErrors::Type::UNKNOWN_LANGUAGE;
      return true;
    case translate::mojom::TranslateError::UNSUPPORTED_LANGUAGE:
      *output = TranslateErrors::Type::UNSUPPORTED_LANGUAGE;
      return true;
    case translate::mojom::TranslateError::IDENTICAL_LANGUAGES:
      *output = TranslateErrors::Type::IDENTICAL_LANGUAGES;
      return true;
    case translate::mojom::TranslateError::TRANSLATION_ERROR:
      *output = TranslateErrors::Type::TRANSLATION_ERROR;
      return true;
    case translate::mojom::TranslateError::TRANSLATION_TIMEOUT:
      *output = TranslateErrors::Type::TRANSLATION_TIMEOUT;
      return true;
    case translate::mojom::TranslateError::UNEXPECTED_SCRIPT_ERROR:
      *output = TranslateErrors::Type::UNEXPECTED_SCRIPT_ERROR;
      return true;
    case translate::mojom::TranslateError::BAD_ORIGIN:
      *output = TranslateErrors::Type::BAD_ORIGIN;
      return true;
    case translate::mojom::TranslateError::SCRIPT_LOAD_ERROR:
      *output = TranslateErrors::Type::SCRIPT_LOAD_ERROR;
      return true;
    case translate::mojom::TranslateError::TRANSLATE_ERROR_MAX:
      *output = TranslateErrors::Type::TRANSLATE_ERROR_MAX;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
bool StructTraits<translate::mojom::LanguageDetectionDetailsDataView,
                  translate::LanguageDetectionDetails>::
    Read(translate::mojom::LanguageDetectionDetailsDataView data,
         translate::LanguageDetectionDetails* out) {
  if (!data.ReadTime(&out->time))
    return false;
  if (!data.ReadUrl(&out->url))
    return false;
  if (!data.ReadContentLanguage(&out->content_language))
    return false;
  if (!data.ReadCldLanguage(&out->cld_language))
    return false;

  out->is_cld_reliable = data.is_cld_reliable();
  out->has_notranslate = data.has_notranslate();

  if (!data.ReadHtmlRootLanguage(&out->html_root_language))
    return false;
  if (!data.ReadAdoptedLanguage(&out->adopted_language))
    return false;
  if (!data.ReadContents(&out->contents))
    return false;

  return true;
}

}  // namespace mojo
