// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/html_based_username_detector.h"

#include <algorithm>
#include <map>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/html_based_username_detector_vocabulary.h"
#include "third_party/WebKit/public/web/WebFormElement.h"

using blink::WebFormControlElement;
using blink::WebFormElement;
using blink::WebInputElement;

namespace autofill {

namespace {

// Minimum length of a word, in order not to be considered short word. Short
// words will not be used in regular string search, because a short word may be
// a part of another word. A short word should be enclosed between delimiters,
// otherwise an occurence doesn't count.
constexpr int kMinimumWordLength = 4;

// For each input element that can be username, developer and user group values
// are computed. The user group value includes what a user sees: label,
// placeholder, aria-label (all are stored in FormFieldData.label). The
// developer group value consists of name and id attribute values.
// For each group the list of short tokens (tokens shorter than
// |kMinimumWordLength|) is computed as well.
struct UsernameFieldData {
  WebInputElement input_element;
  base::string16 developer_value;
  std::vector<base::string16> developer_short_tokens;
  base::string16 user_value;
  std::vector<base::string16> user_short_tokens;
};

// Words that the algorithm looks for are split into multiple categories based
// on feature reliability.
// A category may contain latin dictionary and non-latin dictionary. It is
// mandatory that it has latin one, but non-latin might be missing.
// "Latin" translations are the translations of the words for which the
// original translation is similar to the romanized translation (translation of
// the word only using ISO basic Latin alphabet).
// "Non-latin" translations are the translations of the words that have custom,
// country specific characters.
struct CategoryOfWords {
  const char* const* const latin_dictionary;
  const size_t latin_dictionary_size;
  const char* const* const non_latin_dictionary;
  const size_t non_latin_dictionary_size;
};

// 1. Removes delimiters from |raw_value| and appends it to |*field_data_value|.
// A sentinel symbol is added first if |*field_data_value| is not empty.
// 2. Tokenizes and appends short tokens from from |raw_value| to
// |*field_data_short_tokens|, if any.
void AppendValueAndShortTokens(
    const base::string16& raw_value,
    base::string16* field_data_value,
    std::vector<base::string16>* field_data_short_tokens) {
  // List of separators that can appear in HTML attribute values.
  static const std::string kDelimiters =
      "$\"\'?%*@!\\/&^#:+~`;,>|<.[](){}-_ 0123456789";
  base::string16 lowercase_value = base::i18n::ToLower(raw_value);
  std::vector<base::StringPiece16> tokens =
      base::SplitStringPiece(lowercase_value, base::ASCIIToUTF16(kDelimiters),
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<base::StringPiece16> short_tokens;
  std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(short_tokens),
               [](const base::StringPiece16& token) {
                 return token.size() < kMinimumWordLength;
               });

  for (const base::StringPiece16& token : short_tokens)
    field_data_short_tokens->push_back(token.as_string());

  // Modify |lowercase_value| only when |tokens| has been processed.
  lowercase_value.erase(std::remove_if(
      lowercase_value.begin(), lowercase_value.end(),
      [](char c) { return kDelimiters.find(c) != std::string::npos; }));
  // When computing the developer value, '$' safety guard is being added
  // between field name and id, so that forming of accidental words is
  // prevented.
  if (!field_data_value->empty())
    field_data_value->push_back('$');
  *field_data_value += lowercase_value;
}

// For the given |input_element|, computes developer and user value, along with
// lists of short tokens, and returns it.
UsernameFieldData ComputeFieldData(const blink::WebInputElement& input_element,
                                   const FormFieldData& field) {
  UsernameFieldData field_data;
  field_data.input_element = input_element;

  AppendValueAndShortTokens(field.name, &field_data.developer_value,
                            &field_data.developer_short_tokens);
  AppendValueAndShortTokens(field.id, &field_data.developer_value,
                            &field_data.developer_short_tokens);
  AppendValueAndShortTokens(field.label, &field_data.user_value,
                            &field_data.user_short_tokens);
  return field_data;
}

// For the fields of the given form that can be username fields (form_data and
// all_possible_usernames), computes |UsernameFieldData| needed by the detector.
void InferUsernameFieldData(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    std::vector<UsernameFieldData>* possible_usernames_data) {
  // |all_possible_usernames| and |form_data.fields| may have different set of
  // fields. Match them based on |WebInputElement.NameForAutofill| and
  // |FormFieldData.name|.
  size_t current_index = 0;

  for (const blink::WebInputElement& input_element : all_possible_usernames) {
    for (size_t i = current_index; i < form_data.fields.size(); ++i) {
      const FormFieldData& field = form_data.fields[i];
      if (input_element.NameForAutofill().IsEmpty())
        continue;

      // Find matching form data and web input element.
      if (field.name == input_element.NameForAutofill().Utf16()) {
        current_index = i + 1;
        possible_usernames_data->push_back(
            ComputeFieldData(input_element, field));
        break;
      }
    }
  }
}

// Check if any word from |dictionary| is encountered in computed field
// information (i.e. |value|, |tokens|).
bool CheckFieldWithDictionary(const base::string16& value,
                              const std::vector<base::string16>& tokens,
                              const char* const* dictionary,
                              const size_t& dictionary_size) {
  for (size_t i = 0; i < dictionary_size; ++i) {
    const base::string16 word = base::UTF8ToUTF16(dictionary[i]);
    if (word.length() < kMinimumWordLength) {
      // Treat short words by looking up for them in the tokens list.
      if (std::find(tokens.begin(), tokens.end(), word) != tokens.end())
        return true;
    } else {
      // Treat long words by looking for them as a substring in |value|.
      if (value.find(word) != std::string::npos)
        return true;
    }
  }
  return false;
}

// Check if any word from |category| is encountered in computed field
// information for |possible_username|.
bool ContainsWordFromCategory(const UsernameFieldData& possible_username,
                              const CategoryOfWords& category) {
  // For user value, search in latin and non-latin dictionaries, because this
  // value is user visible. For developer value, only look up in latin
  /// dictionaries.
  return CheckFieldWithDictionary(
             possible_username.user_value, possible_username.user_short_tokens,
             category.latin_dictionary, category.latin_dictionary_size) ||
         CheckFieldWithDictionary(possible_username.user_value,
                                  possible_username.user_short_tokens,
                                  category.non_latin_dictionary,
                                  category.non_latin_dictionary_size) ||
         CheckFieldWithDictionary(possible_username.developer_value,
                                  possible_username.developer_short_tokens,
                                  category.latin_dictionary,
                                  category.latin_dictionary_size);
}

// Remove from |possible_usernames_data| the elements that definitely cannot be
// usernames, because their computed values contain at least one negative word.
void RemoveFieldsWithNegativeWords(
    std::vector<UsernameFieldData>* possible_usernames_data) {
  static const CategoryOfWords kNegativeCategory{
      kNegativeLatin, kNegativeLatinSize, kNegativeNonLatin,
      kNegativeNonLatinSize};

  possible_usernames_data->erase(
      std::remove_if(possible_usernames_data->begin(),
                     possible_usernames_data->end(),
                     [](const UsernameFieldData& possible_username) {
                       return ContainsWordFromCategory(possible_username,
                                                       kNegativeCategory);
                     }),
      possible_usernames_data->end());
}

// Check if any word from the given category (|category|) appears in fields from
// the form (|possible_usernames_data|). If the category words appear in more
// than 2 fields, do not make a decision, because it may just be a prefix. If
// the words appears in 1 or 2 fields, the first field is saved to
// |*username_element|.
bool FormContainsWordFromCategory(
    const std::vector<UsernameFieldData>& possible_usernames_data,
    const CategoryOfWords& category,
    WebInputElement* username_element) {
  // Auxiliary element that contains the first field (in order of appearance in
  // the form) in which a substring is encountered.
  WebInputElement chosen_field;

  size_t count = 0;
  for (const UsernameFieldData& field_data : possible_usernames_data) {
    if (ContainsWordFromCategory(field_data, category)) {
      if (count == 0)
        chosen_field = field_data.input_element;
      count++;
    }
  }

  if (count && count <= 2) {
    *username_element = chosen_field;
    return true;
  } else {
    return false;
  }
}

// Find username element if there is no cached result for the given form.
bool FindUsernameFieldInternal(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    WebInputElement* username_element) {
  DCHECK(username_element);

  static const CategoryOfWords kUsernameCategory{
      kUsernameLatin, kUsernameLatinSize, kUsernameNonLatin,
      kUsernameNonLatinSize};
  static const CategoryOfWords kUserCategory{kUserLatin, kUserLatinSize,
                                             kUserNonLatin, kUserNonLatinSize};
  static const CategoryOfWords kTechnicalCategory{
      kTechnicalWords, kTechnicalWordsSize, nullptr, 0};
  static const CategoryOfWords kWeakCategory{kWeakWords, kWeakWordsSize,
                                             nullptr, 0};
  // These categories contain words that point to username field.
  static const CategoryOfWords kPositiveCategories[] = {
      kUsernameCategory, kUserCategory, kTechnicalCategory, kWeakCategory};

  std::vector<UsernameFieldData> possible_usernames_data;
  InferUsernameFieldData(all_possible_usernames, form_data,
                         &possible_usernames_data);
  RemoveFieldsWithNegativeWords(&possible_usernames_data);

  // These are the searches performed by the username detector.
  // Order of categories is vital: the detector searches for words in descending
  // order of probability to point to a username field.
  for (const CategoryOfWords& category : kPositiveCategories) {
    if (FormContainsWordFromCategory(possible_usernames_data, category,
                                     username_element)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool GetUsernameFieldBasedOnHtmlAttributes(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    WebInputElement* username_element,
    UsernameDetectorCache* username_detector_cache) {
  DCHECK(username_element);

  if (all_possible_usernames.empty())
    return false;

  const blink::WebFormElement form = all_possible_usernames[0].Form();
  if (!username_detector_cache ||
      username_detector_cache->find(form) == username_detector_cache->end()) {
    bool username_found = FindUsernameFieldInternal(
        all_possible_usernames, form_data, username_element);
    if (username_detector_cache) {
      (*username_detector_cache)[form] =
          username_found ? *username_element : blink::WebInputElement();
    }
    return username_found;
  } else {  // Use the cached value for |form|.
    *username_element = (*username_detector_cache)[form];
    return !username_element->IsNull();
  }
}

}  // namespace autofill
