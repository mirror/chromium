// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/json_classifier.h"

#include "base/containers/adapters.h"
#include "base/logging.h"

using base::StringPiece;

namespace content {

enum JsonClassifier::State : uint8_t {
  // Whitespace is ignored in any of these.
  kIsNotLegalJson,
  kIsCompleteLegalJson,
  kAnyValue,
  kStringValue,
  kOpenList,
  kInList,
  kOpenDict,
  kInDict,
  kColonAfterKey,

  // States for string literals.
  kInStringEscape,
  kInString,
  kInStringHexEscape1,
  kInStringHexEscape2,
  kInStringHexEscape3,
  kInStringHexEscape4,

  // States for number literals.
  kDigitsOrDecimalOrExponentOrDone,
  kDigitsOrExponentOrDone,
  kDigitsOrDone,
  kDecimalOrExponentOrDone,
  kExpectFirstDigit,
  kDigitsThenExponentOrDone,
  kPlusOrMinusOrDigits,
  kDigitsThenDone,

  // States for 'true'
  kLiteralRUE,
  kLiteralUE,
  kLiteralE,

  // States for 'false'
  kLiteralALSE,
  kLiteralLSE,
  kLiteralSE,

  // States for 'null'
  kLiteralULL,
  kLiteralLL,
  kLiteralL,
};

JsonClassifier::JsonClassifier()
    : is_not_valid_javascript_(false), state_(kAnyValue) {
  Push(kIsCompleteLegalJson);
}

JsonClassifier::~JsonClassifier() {}

bool JsonClassifier::is_complete_json_value() const {
  switch (state_) {
    case kIsCompleteLegalJson:
      return true;
    case kDigitsOrDecimalOrExponentOrDone:
    case kDigitsOrExponentOrDone:
    case kDigitsOrDone:
    case kDecimalOrExponentOrDone:
      return (stack_->size() == 1);
    default:
      return false;
  }
}

JsonClassifier* JsonClassifier::Append(base::StringPiece data) {
  for (size_t i = 0; i < data.length(); ++i) {
    const char c = data[i];

    switch (state_) {
      case kIsNotLegalJson:
      case kAnyValue:
      case kStringValue:
      case kOpenList:
      case kInList:
      case kOpenDict:
      case kInDict:
      case kColonAfterKey:
      case kIsCompleteLegalJson:
        // Whitespace is ignored in these contexts.
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\x0d')
          continue;
        break;
      case kInStringEscape:
      case kInString:
        // Handle illegal chars.
        if ((c >= 0 && c < 32) || c == 127)
          return Fail();
        break;
      case kDigitsOrDecimalOrExponentOrDone:
      case kDigitsOrExponentOrDone:
      case kDigitsOrDone:
        // Digits in these cases are consumed without changing the state.
        if ('0' <= c && c <= '9')
          continue;
        break;
      default:
        break;
    }

    // Main switch statement.
    switch (state_) {
      case kIsNotLegalJson:
      case kIsCompleteLegalJson:
        // After we finished our top-level value, only whitespace is allowed.
        return Fail();
      case kDigitsOrDecimalOrExponentOrDone:  // Handle decimal cases.
      case kDecimalOrExponentOrDone:
        if (c == '.') {
          state_ = kDigitsThenExponentOrDone;
          continue;
        }
      // fall through to exponent handling.
      case kDigitsOrExponentOrDone:  // Handle exponent cases.
        if (c == 'e' || c == 'E') {
          state_ = kPlusOrMinusOrDigits;
          continue;
        }
      // Fall through to Done.
      case kDigitsOrDone:
        state_ = Pop();  // Done with numeric value; c unconsumed.
        --i;             // Since we didn't consume c, rewind.
        continue;
      case kDigitsThenExponentOrDone:
        if ('0' <= c && c <= '9') {
          state_ = kDigitsOrExponentOrDone;
          continue;
        }
        return Fail();
      case kPlusOrMinusOrDigits:
        if (c == '+' || c == '-') {
          state_ = kDigitsThenDone;
          continue;
        }
      // Fall through.
      case kDigitsThenDone:
        if ('0' <= c && c <= '9') {
          state_ = kDigitsOrDone;
          continue;
        }
        return Fail();
      case kOpenDict:
        if (c == '}') {
          state_ = Pop();  // Completes an empty object.
        } else if (c == '\"') {
          state_ = kInString;
          Push(kColonAfterKey);
        } else {
          return Fail();
        }
        continue;
      case kStringValue:
        if (c == '"') {
          state_ = kInString;
          continue;
        }
        return Fail();
      case kInString:
        if (c == '"')
          state_ = Pop();  // this completes the string value.
        else if (c == '\\')
          state_ = kInStringEscape;
        continue;
      case kInStringHexEscape4:
      case kInStringHexEscape3:
      case kInStringHexEscape2:
      case kInStringHexEscape1:
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
            ('A' <= c && c <= 'F')) {
          DCHECK_EQ(kInStringHexEscape1 - 1, kInString);
          state_ = static_cast<State>(state_ - 1);
          continue;
        }
        return Fail();
      case kInStringEscape:
        switch (c) {
          case '"':
          case '\\':
          case '/':
          case 'b':
          case 'f':
          case 'n':
          case 'r':
          case 't':
            state_ = kInString;
            continue;
          case 'u':
            state_ = kInStringHexEscape4;
            continue;
          default:
            return Fail();
        }
      case kOpenList:
        if (c == ']') {
          state_ = Pop();  // Completes an empty list.
          continue;
        }
        state_ = kAnyValue;
        if (!Push(kInList))
          return Fail();
      // Fall through.
      case kAnyValue:
        switch (c) {
          case '[':
            state_ = kOpenList;          // Start of array.
            continue;                    //
          case '{':                      //
            state_ = kOpenDict;          // Start of object.
            continue;                    //
          case 't':                      //
            state_ = kLiteralRUE;        // Start of true.
            continue;                    //
          case 'f':                      //
            state_ = kLiteralALSE;       // Start of false.
            continue;                    //
          case 'n':                      //
            state_ = kLiteralULL;        // Start of null.
            continue;                    //
          case '"':                      //
            state_ = kInString;          // Start of string.
            continue;                    //
          case '-':                      //
            state_ = kExpectFirstDigit;  // Start of negative number.
            continue;
        }
      // "any value" falls through to "numeric value".
      case kExpectFirstDigit:  // Start of number.
        if ('1' <= c && c <= '9')
          state_ = kDigitsOrDecimalOrExponentOrDone;
        else if (c == '0')
          state_ = kDecimalOrExponentOrDone;
        else
          return Fail();
        continue;
      case kLiteralRUE:
        if (c != 'r')
          return Fail();
        state_ = kLiteralUE;
        continue;
      case kLiteralUE:
        if (c != 'u')
          return Fail();
        state_ = kLiteralE;
        continue;
      case kLiteralE:
        if (c != 'e')
          return Fail();
        state_ = Pop();  // Completes a 'true' or 'false' literal.
        continue;
      case kLiteralALSE:
        if (c != 'a')
          return Fail();
        state_ = kLiteralLSE;
        continue;
      case kLiteralLSE:
        if (c != 'l')
          return Fail();
        state_ = kLiteralSE;
        continue;
      case kLiteralSE:
        if (c != 's')
          return Fail();
        state_ = kLiteralE;
        continue;
      case kLiteralULL:
        if (c != 'u')
          return Fail();
        state_ = kLiteralLL;
        continue;
      case kLiteralLL:
        if (c != 'l')
          return Fail();
        state_ = kLiteralL;
        continue;
      case kLiteralL:
        if (c != 'l')
          return Fail();
        state_ = Pop();  // Completes a 'null' literal.
        continue;
      case kColonAfterKey:
        if (c != ':')
          return Fail();
        // The colon in a top-level JSON dictionary is a Javascript syntax error
        // if interpreted as such. (Other cases of valid JSON, including lists
        // and empty dicts, are legal Javascript syntax).
        if (stack_->size() == 1)
          is_not_valid_javascript_ = true;
        state_ = kAnyValue;
        Push(kInDict);
        continue;
      case kInDict:
        if (c == ',') {
          state_ = kStringValue;
          Push(kColonAfterKey);
        } else if (c == '}') {
          state_ = Pop();
        } else {
          return Fail();
        }
        continue;
      case kInList:
        if (c == ',') {
          state_ = kAnyValue;
          Push(kInList);
        } else if (c == ']') {
          state_ = Pop();
        } else {
          return Fail();
        }
        continue;
    }
  }
  DCHECK(is_valid_json_so_far());
  return this;
}

std::string JsonClassifier::GetCompletionSuffixForTesting() const {
  std::string result;
  State state = state_;
  for (State next_state : base::Reversed(stack_.container())) {
    switch (state) {
      case kIsNotLegalJson:
        return "";
      case kIsCompleteLegalJson:
        return "";
      case kAnyValue:
        result.append("9");
        break;
      case kStringValue:
        result.append("\"\"");
        break;
      case kOpenList:
      case kInList:
        result.append("]");
        break;
      case kOpenDict:
      case kInDict:
        result.append("}");
        break;
      case kColonAfterKey:
        result.append(":8}");
        break;
      case kInString:
        result.append("\"");
        break;
      case kInStringEscape:
        result.append("\\\"");
        break;
      case kInStringHexEscape4:
        result.append("1b2c\"");
        break;
      case kInStringHexEscape3:
        result.append("0ff\"");
        break;
      case kInStringHexEscape2:
        result.append("3f\"");
        break;
      case kInStringHexEscape1:
        result.append("A\"");
        break;
      case kDigitsOrDecimalOrExponentOrDone:
      case kDigitsOrExponentOrDone:
      case kDigitsOrDone:
      case kDecimalOrExponentOrDone:
        break;
      case kExpectFirstDigit:
      case kDigitsThenDone:
      case kDigitsThenExponentOrDone:
      case kPlusOrMinusOrDigits:
        result.append("7");
        break;
      case kLiteralRUE:
        result.append("rue");
        break;
      case kLiteralUE:
        result.append("ue");
        break;
      case kLiteralE:
        result.append("e");
        break;
      case kLiteralALSE:
        result.append("alse");
        break;
      case kLiteralLSE:
        result.append("lse");
        break;
      case kLiteralSE:
        result.append("se");
        break;
      case kLiteralULL:
        result.append("ull");
        break;
      case kLiteralLL:
        result.append("ll");
        break;
      case kLiteralL:
        result.append("l");
        break;
    }
    state = next_state;
  }
  return result;
}

bool JsonClassifier::Push(State state_to_push) {
  if (stack_->size() < 2000) {
    stack_->push_back(state_to_push);
    return true;
  }
  {
    Fail();
    return false;
  }
}

JsonClassifier::State JsonClassifier::Pop() {
  JsonClassifier::State result = stack_->back();
  stack_->pop_back();
  return result;
}

JsonClassifier* JsonClassifier::Fail() {
  stack_->clear();
  state_ = kIsNotLegalJson;
  return this;
}

}  // namespace content
