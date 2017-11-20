// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/credit_card_field.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_scanner.h"
#include "components/autofill/core/browser/field_filler.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_regex_constants.h"
#include "components/autofill/core/common/autofill_regexes.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {

namespace {

// Credit card numbers are at most 19 digits in length.
// [Ref: http://en.wikipedia.org/wiki/Bank_card_number]
const size_t kMaxValidCardNumberSize = 19;

// Look for the vector |regex_needles| in |haystack|. Returns true if a
// consecutive section of |haystack| matches |regex_needles|.
bool FindConsecutiveStrings(const std::vector<base::string16>& regex_needles,
                            const std::vector<base::string16>& haystack) {
  if (regex_needles.empty() ||
      haystack.empty() ||
      (haystack.size() < regex_needles.size()))
    return false;

  for (size_t i = 0; i < haystack.size() - regex_needles.size() + 1; ++i) {
    for (size_t j = 0; j < regex_needles.size(); ++j) {
      if (!MatchesPattern(haystack[i + j], regex_needles[j]))
        break;

      if (j == regex_needles.size() - 1)
        return true;
    }
  }
  return false;
}

// Returns true if a field that has |max_length| can fit the data for a field of
// |type|.
bool FieldCanFitDataForFieldType(int max_length, ServerFieldType type) {
  if (max_length == 0)
    return true;

  switch (type) {
    case CREDIT_CARD_EXP_DATE_2_DIGIT_YEAR: {
      // A date with a 2 digit year can fit in a minimum of 4 chars (MMYY)
      static constexpr int kMinimum2YearCcExpLength = 4;
      return max_length >= kMinimum2YearCcExpLength;
    }
    case CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR: {
      // A date with a 4 digit year can fit in a minimum of 6 chars (MMYYYY)
      static constexpr int kMinimum4YearCcExpLength = 6;
      return max_length >= kMinimum4YearCcExpLength;
    }
    default:
      NOTREACHED();
      return false;
  }
}

}  // namespace

// static
std::unique_ptr<FormField> CreditCardField::Parse(AutofillScanner* scanner) {
  if (scanner->IsEnd())
    return nullptr;

  std::unique_ptr<CreditCardField> credit_card_field(new CreditCardField);

  // A cursor saving the field position before parsing. Scanner will be set to
  // this field after a failed parsing.
  size_t saved_cursor = scanner->SaveCursor();

  // A cursor saving the field position that is directly after the form. Scanner
  // will be set to this field after a successful parsing.
  size_t cursor_after_form = saved_cursor;

  // True if the previous loop's field was skipped.
  bool just_skipped = false;
  // Max number of skipped fields in this form.
  // This number must be at least 2 as forms can have hidden field to handle
  // month and year. The number must not be too big to avoid parsing multiple
  // forms in the same pass.
  // int max_skips = 3;

  // True if the parsing is currently parsing credit card numbers.
  // It is possible for forms to have multiple credit card number fields (by
  // 4 digits, or a confirmation field). This is only allowed as consecutive
  // fields.
  bool parsing_credit_card_numbers = false;
  bool just_parsed_credit_card_number = false;

  // Credit card fields can appear in many different orders.
  // We loop until no more credit card related fields are found, see |break| at
  // the bottom of the loop.
  for (int fields = 0;
       !scanner->IsEnd() && scanner->SaveCursor() - saved_cursor < 10;
       ++fields) {
    if (!just_skipped) {
      // The last field was parsed successfully on the previous loop and the
      // cursor now points to the first field after the form.
      cursor_after_form = scanner->SaveCursor();
      parsing_credit_card_numbers = just_parsed_credit_card_number;
    }
    just_parsed_credit_card_number = false;
    just_skipped = false;

    // Ignore gift card fields.
    if (IsGiftCardField(scanner))
      break;

    if (!credit_card_field->HasFocusable(credit_card_field->cardholder_)) {
      if (ParseFieldSpecificsAllowingReplaceAndSkips(
              scanner, base::UTF8ToUTF16(kNameOnCardRe), MATCH_DEFAULT, 0, true,
              &credit_card_field->cardholder_)) {
        continue;
      }

      // Sometimes the cardholder field is just labeled "name". Unfortunately
      // this is a dangerously generic word to search for, since it will often
      // match a name (not cardholder name) field before or after credit card
      // fields. So we search for "name" only when we've already parsed at
      // least one other credit card field and haven't yet parsed the
      // expiration date (which usually appears at the end).
      if (fields > 0 && !credit_card_field->HasExpiration() &&
          ParseFieldSpecificsAllowingReplaceAndSkips(
              scanner, base::UTF8ToUTF16(kNameOnCardContextualRe),
              MATCH_DEFAULT, 0, true, &credit_card_field->cardholder_)) {
        continue;
      }
    }
    if (credit_card_field->cardholder_ &&
        !credit_card_field->HasFocusable(credit_card_field->cardholder_last_)) {
      // Search for a last name. Since this is a dangerously generic search, we
      // execute it only after we have found a valid credit card (first) name
      // and haven't yet parsed the expiration date (which usually appears at
      // the end).
      if (!credit_card_field->HasExpiration() &&
          ParseFieldSpecificsAllowingReplaceAndSkips(
              scanner, base::UTF8ToUTF16(kLastNameRe), MATCH_DEFAULT, 0, true,
              &credit_card_field->cardholder_last_)) {
        continue;
      }
    }

    // Check for a credit card type (Visa, Mastercard, etc.) field.
    // All CC type fields encountered so far have been of type select.
    if (!credit_card_field->HasFocusable(credit_card_field->type_) &&
        LikelyCardTypeSelectField(scanner)) {
      credit_card_field->ConsiderNewCandidate(&credit_card_field->type_,
                                              scanner->Cursor());
      scanner->Advance();
      continue;
    }

    // We look for a card security code before we look for a credit card number
    // and match the general term "number". The security code has a plethora of
    // names; we've seen "verification #", "verification number", "card
    // identification number", and others listed in the regex pattern used
    // below.
    // Note: Some sites use type="tel" or type="number" for numerical inputs.
    // They also sometimes use type="password" for sensitive types.
    const int kMatchNumTelAndPwd =
        MATCH_DEFAULT | MATCH_NUMBER | MATCH_TELEPHONE | MATCH_PASSWORD;
    if (!credit_card_field->HasFocusable(credit_card_field->verification_) &&
        ParseFieldSpecificsAllowingReplaceAndSkips(
            scanner, base::UTF8ToUTF16(kCardCvcRe), kMatchNumTelAndPwd, 0, true,
            &credit_card_field->verification_)) {
      // A couple of sites have multiple verification codes right after another.
      // Allow the classification of these codes one by one.
      AutofillField* second_cvv = nullptr;

      // Check if the verification code is the first detected field in the newly
      // started card.
      if (credit_card_field->numbers_.empty() &&
          !credit_card_field->HasExpiration() &&
          !credit_card_field->cardholder_ && scanner->SaveCursor() > 1) {
        // Check if the previous field was a verification code.
        scanner->RewindTo(scanner->SaveCursor() - 2);
        if (ParseFieldSpecificsAllowingReplaceAndSkips(
                scanner, base::UTF8ToUTF16(kCardCvcRe), kMatchNumTelAndPwd, 0,
                false, &second_cvv)) {
          // Put the scanner back to the field right after the current cvv.
          scanner->Advance();
          return std::move(credit_card_field);
        } else {
          // Put the scanner back to the field right after the current cvv.
          scanner->Advance();
          scanner->Advance();
        }
      }

      continue;
    }

    // TODO(crbug.com/591816): Make sure parsing cc-numbers of type password
    // doesn't have bad side effects.
    AutofillField* current_number_field = nullptr;
    if (ParseFieldSpecificsAllowingReplaceAndSkips(
            scanner, base::UTF8ToUTF16(kCardNumberRe), kMatchNumTelAndPwd, 0,
            true, &current_number_field)) {
      // Avoid autofilling any credit card number field having very low or high
      // |start_index| on the HTML form.
      size_t start_index = 0;
      if (credit_card_field->numbers_.empty()) {
        // This is the first credit card number found in the page. This mark the
        // begining of the credit card number section that can contain multiple
        // credit card number (full or partial).
        parsing_credit_card_numbers = true;
      }
      if (parsing_credit_card_numbers) {
        // Only allow credit card numbers in the credit card number section.
        // This mean that either this field is the first credit card number, or
        // previous field parsed was also one.
        if (!credit_card_field->numbers_.empty()) {
          // Previous fields containing card number have already be found.
          // Check if there are multiple copies of the full number or if all
          // fields are part of the same number input.
          size_t last_number_field_size =
              credit_card_field->numbers_.back()->credit_card_number_offset() +
              credit_card_field->numbers_.back()->max_length;

          // Distinguish between
          //   (a) one card split across multiple fields
          //   (b) multiple fields for multiple cards
          // Treat this field as a part of the same card as the last field,
          // except when doing so would cause overflow.
          if (last_number_field_size < kMaxValidCardNumberSize)
            start_index = last_number_field_size;
        }
        current_number_field->set_credit_card_number_offset(start_index);
        credit_card_field->numbers_.push_back(current_number_field);
        just_parsed_credit_card_number = true;
        continue;
      }
    }

    if (credit_card_field->ParseExpirationDate(scanner))
      continue;

    if (credit_card_field->expiration_month_ &&
        !credit_card_field->expiration_year_ &&
        !credit_card_field->expiration_date_) {
      // Parsed a month but couldn't parse a year; give up.
      scanner->RewindTo(saved_cursor);
      return nullptr;
    }
    // All parsing failed. The current field will be be skipped or parsing will
    // stop.
    just_skipped = true;
    if (fields > 0 && !scanner->IsEnd() && !scanner->Cursor()->is_focusable) {
      // If the current could not be parsed but is not focusable, it is possibly
      // a technical field.
      // If a form is already parsed (fields > 0) and the limit of passed fields
      // is not reached, allow the parsing to continue ignoring this field.
      scanner->Advance();
      continue;
    }
    break;
  }

  if (just_skipped) {
    // The last fields were ignored, rewind the cursor to the first unparsed
    // field.
    scanner->RewindTo(cursor_after_form);
  }
  int form_range = cursor_after_form - saved_cursor;
  int used_fields = credit_card_field->numbers_.size() +
                    (credit_card_field->cardholder_ ? 1 : 0) +
                    (credit_card_field->cardholder_last_ ? 1 : 0) +
                    (credit_card_field->type_ ? 1 : 0) +
                    (credit_card_field->verification_ ? 1 : 0) +
                    (credit_card_field->expiration_month_ ? 1 : 0) +
                    (credit_card_field->expiration_year_ ? 1 : 0) +
                    (credit_card_field->expiration_date_ ? 1 : 0);
  if (form_range - used_fields > 3 || form_range >= 2 * used_fields) {
    scanner->RewindTo(saved_cursor);
    return nullptr;
  }

  // Some pages have a billing address field after the cardholder name field.
  // For that case, allow only just the cardholder name field.  The remaining
  // CC fields will be picked up in a following CreditCardField.
  if (credit_card_field->cardholder_)
    return std::move(credit_card_field);

  // On some pages, the user selects a card type using radio buttons
  // (e.g. test page Apple Store Billing.html).  We can't handle that yet,
  // so we treat the card type as optional for now.
  // The existence of a number or cvc in combination with expiration date is
  // a strong enough signal that this is a credit card.  It is possible that
  // the number and name were parsed in a separate part of the form.  So if
  // the cvc and date were found independently they are returned.
  const bool has_cc_number_or_verification =
      (credit_card_field->verification_ ||
       !credit_card_field->numbers_.empty());
  if (has_cc_number_or_verification && credit_card_field->HasExpiration())
    return std::move(credit_card_field);

  scanner->RewindTo(saved_cursor);
  return nullptr;
}

// static
bool CreditCardField::LikelyCardMonthSelectField(AutofillScanner* scanner) {
  if (scanner->IsEnd())
    return false;

  AutofillField* field = scanner->Cursor();
  if (!MatchesFormControlType(field->form_control_type, MATCH_SELECT))
    return false;

  if (field->option_values.size() < 12 || field->option_values.size() > 13)
    return false;

  // Filter out years.
  const base::string16 kNumericalYearRe =
      base::ASCIIToUTF16("[1-9][0-9][0-9][0-9]");
  for (const auto& value : field->option_values) {
    if (MatchesPattern(value, kNumericalYearRe))
      return false;
  }
  for (const auto& value : field->option_contents) {
    if (MatchesPattern(value, kNumericalYearRe))
      return false;
  }

  // Look for numerical months.
  const base::string16 kNumericalMonthRe = base::ASCIIToUTF16("12");
  if (MatchesPattern(field->option_values.back(), kNumericalMonthRe) ||
      MatchesPattern(field->option_contents.back(), kNumericalMonthRe)) {
    return true;
  }

  // Maybe do more matches here. e.g. look for (translated) December.

  // Unsure? Return false.
  return false;
}

// static
bool CreditCardField::LikelyCardYearSelectField(AutofillScanner* scanner) {
  if (scanner->IsEnd())
    return false;

  AutofillField* field = scanner->Cursor();
  if (!MatchesFormControlType(field->form_control_type, MATCH_SELECT))
    return false;

  const base::Time time_now = AutofillClock::Now();
  base::Time::Exploded time_exploded;
  time_now.UTCExplode(&time_exploded);

  const int kYearsToMatch = 3;
  std::vector<base::string16> years_to_check;
  for (int year = time_exploded.year;
       year < time_exploded.year + kYearsToMatch;
       ++year) {
    years_to_check.push_back(base::IntToString16(year));
  }
  return (FindConsecutiveStrings(years_to_check, field->option_values) ||
          FindConsecutiveStrings(years_to_check, field->option_contents));
}

// static
bool CreditCardField::LikelyCardTypeSelectField(AutofillScanner* scanner) {
  if (scanner->IsEnd())
    return false;

  AutofillField* field = scanner->Cursor();

  if (!MatchesFormControlType(field->form_control_type, MATCH_SELECT))
    return false;

  // We set |ignore_whitespace| to true on these calls because this is actually
  // a pretty common mistake; e.g., "Master card" instead of "Mastercard".
  bool isSelect = (FieldFiller::FindShortestSubstringMatchInSelect(
                       l10n_util::GetStringUTF16(IDS_AUTOFILL_CC_VISA), true,
                       field) >= 0) ||
                  (FieldFiller::FindShortestSubstringMatchInSelect(
                       l10n_util::GetStringUTF16(IDS_AUTOFILL_CC_MASTERCARD),
                       true, field) >= 0);
  return isSelect;
}

// static
bool CreditCardField::IsGiftCardField(AutofillScanner* scanner) {
  if (scanner->IsEnd())
    return false;

  size_t saved_cursor = scanner->SaveCursor();
  if (ParseField(scanner, base::UTF8ToUTF16(kDebitCardRe), nullptr)) {
    scanner->RewindTo(saved_cursor);
    return false;
  }
  if (ParseField(scanner, base::UTF8ToUTF16(kDebitGiftCardRe), nullptr)) {
    scanner->RewindTo(saved_cursor);
    return false;
  }

  return ParseField(scanner, base::UTF8ToUTF16(kGiftCardRe), nullptr);
}

CreditCardField::CreditCardField()
    : cardholder_(nullptr),
      cardholder_last_(nullptr),
      type_(nullptr),
      verification_(nullptr),
      expiration_month_(nullptr),
      expiration_year_(nullptr),
      expiration_date_(nullptr),
      exp_year_type_(CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR) {
}

CreditCardField::~CreditCardField() {
}

void CreditCardField::AddClassifications(
    FieldCandidatesMap* field_candidates) const {
  for (size_t index = 0; index < numbers_.size(); ++index) {
    AddClassification(numbers_[index], CREDIT_CARD_NUMBER,
                      kBaseCreditCardParserScore, field_candidates);
  }

  AddClassification(type_, CREDIT_CARD_TYPE, kBaseCreditCardParserScore,
                    field_candidates);
  AddClassification(verification_, CREDIT_CARD_VERIFICATION_CODE,
                    kBaseCreditCardParserScore, field_candidates);

  // If the heuristics detected first and last name in separate fields,
  // then ignore both fields. Putting them into separate fields is probably
  // wrong, because the credit card can also contain a middle name or middle
  // initial.
  if (cardholder_last_ == nullptr) {
    AddClassification(cardholder_, CREDIT_CARD_NAME_FULL,
                      kBaseCreditCardParserScore, field_candidates);
  } else {
    AddClassification(cardholder_, CREDIT_CARD_NAME_FIRST,
                      kBaseCreditCardParserScore, field_candidates);
    AddClassification(cardholder_last_, CREDIT_CARD_NAME_LAST,
                      kBaseCreditCardParserScore, field_candidates);
  }

  if (expiration_date_) {
    DCHECK(!expiration_month_);
    DCHECK(!expiration_year_);
    AddClassification(expiration_date_, GetExpirationYearType(),
                      kBaseCreditCardParserScore, field_candidates);
  } else {
    AddClassification(expiration_month_, CREDIT_CARD_EXP_MONTH,
                      kBaseCreditCardParserScore, field_candidates);
    AddClassification(expiration_year_, GetExpirationYearType(),
                      kBaseCreditCardParserScore, field_candidates);
  }
}

bool CreditCardField::ParseExpirationDate(AutofillScanner* scanner) {
  if (!HasFocusable(expiration_date_) &&
      base::LowerCaseEqualsASCII(scanner->Cursor()->form_control_type,
                                 "month")) {
    if (ConsiderNewExpiration(nullptr, nullptr, scanner->Cursor())) {
      scanner->Advance();
      return true;
    } else {
      // The calendar won't be used as a new expiration date (it is likely not
      // focusable. There is no need to continue the parsing.
      return false;
    }
  }

  if (HasFocusableExpiration())
    return false;

  // The new candidates for expiration date.
  AutofillField* expiration_date = nullptr;
  AutofillField* expiration_month = nullptr;
  AutofillField* expiration_year = nullptr;

  // First try to parse split month/year expiration fields by looking for a
  // pair of select fields that look like month/year.
  size_t month_year_saved_cursor = scanner->SaveCursor();

  if (LikelyCardMonthSelectField(scanner)) {
    expiration_month = scanner->Cursor();
    scanner->Advance();
    bool likely_year = LikelyCardYearSelectField(scanner);
    if (!likely_year && !scanner->Cursor()->is_focusable) {
      scanner->Advance();
      likely_year = LikelyCardYearSelectField(scanner);
    }
    if (likely_year) {
      expiration_year = scanner->Cursor();

      if (ConsiderNewExpiration(expiration_month, expiration_year,
                                expiration_date)) {
        scanner->Advance();
        return true;
      }
    }
  }

  // If that fails, do a general regex search.
  scanner->RewindTo(month_year_saved_cursor);
  expiration_month = nullptr;
  expiration_year = nullptr;
  expiration_date = nullptr;
  const int kMatchNumAndTelAndSelect =
      MATCH_DEFAULT | MATCH_NUMBER | MATCH_TELEPHONE | MATCH_SELECT;
  if (ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::UTF8ToUTF16(kExpirationMonthRe),
          kMatchNumAndTelAndSelect, 0, true, &expiration_month) &&
      ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::UTF8ToUTF16(kExpirationYearRe),
          kMatchNumAndTelAndSelect, 1, true, &expiration_year) &&
      ConsiderNewExpiration(expiration_month, expiration_year,
                            expiration_date)) {
    return true;
  }

  // If that fails, look for just MM and/or YY(YY).
  scanner->RewindTo(month_year_saved_cursor);
  expiration_month = nullptr;
  expiration_year = nullptr;
  expiration_date = nullptr;
  if (ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::ASCIIToUTF16("^mm$"), kMatchNumAndTelAndSelect, 0,
          true, &expiration_month) &&
      ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::ASCIIToUTF16("^(yy|yyyy)$"), kMatchNumAndTelAndSelect,
          1, true, &expiration_year) &&
      ConsiderNewExpiration(expiration_month, expiration_year,
                            expiration_date)) {
    return true;
  }

  // If that fails, try to parse a combined expiration field.
  // We allow <select> fields, because they're used e.g. on qvc.com.
  scanner->RewindTo(month_year_saved_cursor);
  expiration_month = nullptr;
  expiration_year = nullptr;
  expiration_date = nullptr;

  // Bail out if the field cannot fit a 2-digit year expiration date.
  const int current_field_max_length = scanner->Cursor()->max_length;
  if (!FieldCanFitDataForFieldType(current_field_max_length,
                                   CREDIT_CARD_EXP_DATE_2_DIGIT_YEAR))
    return false;

  // Try to look for a 2-digit year expiration date.
  if (ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::UTF8ToUTF16(kExpirationDate2DigitYearRe),
          kMatchNumAndTelAndSelect, 0, true, &expiration_date) &&
      ConsiderNewExpiration(expiration_month, expiration_year,
                            expiration_date)) {
    exp_year_type_ = CREDIT_CARD_EXP_DATE_2_DIGIT_YEAR;
    return true;
  }

  // Try to look for a generic expiration date field. (2 or 4 digit year)
  if (ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::UTF8ToUTF16(kExpirationDateRe),
          kMatchNumAndTelAndSelect, 0, true, &expiration_date) &&
      ConsiderNewExpiration(expiration_month, expiration_year,
                            expiration_date)) {
    // If such a field exists, but it cannot fit a 4-digit year expiration
    // date, then the likely possibility is that it is a 2-digit year expiration
    // date.
    if (!FieldCanFitDataForFieldType(current_field_max_length,
                                     CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR)) {
      exp_year_type_ = CREDIT_CARD_EXP_DATE_2_DIGIT_YEAR;
    }
    return true;
  }

  // Try to look for a 4-digit year expiration date.
  if (FieldCanFitDataForFieldType(current_field_max_length,
                                  CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR) &&
      ParseFieldSpecificsAllowingReplaceAndSkips(
          scanner, base::UTF8ToUTF16(kExpirationDate4DigitYearRe),
          kMatchNumAndTelAndSelect, 0, true, &expiration_date) &&
      ConsiderNewExpiration(expiration_month, expiration_year,
                            expiration_date)) {
    return true;
  }

  return false;
}

ServerFieldType CreditCardField::GetExpirationYearType() const {
  return (expiration_date_
              ? exp_year_type_
              : ((expiration_year_ && expiration_year_->max_length == 2)
                     ? CREDIT_CARD_EXP_2_DIGIT_YEAR
                     : CREDIT_CARD_EXP_4_DIGIT_YEAR));
}

bool CreditCardField::HasExpiration() const {
  return expiration_date_ || (expiration_month_ && expiration_year_);
}

bool CreditCardField::HasFocusable(AutofillField* field) {
  return field && field->is_focusable;
}

bool CreditCardField::ConsiderNewCandidate(AutofillField** target,
                                           AutofillField* candidate) {
  if (!candidate) {
    return false;
  }
  if (!(*target) || (!(*target)->is_focusable && candidate->is_focusable)) {
    *target = candidate;
    return true;
  }
  return false;
}

bool CreditCardField::ConsiderNewExpiration(AutofillField* month_candidate,
                                            AutofillField* year_candidate,
                                            AutofillField* date_candidate) {
  // If month is provided, year must also be provided.
  DCHECK(!month_candidate == !year_candidate);
  // Either (month and date) or year must be provided. Not all of them.
  DCHECK(!month_candidate || !date_candidate);
  // Possible cases cases:
  // 1 Candidate is null
  // 2 Previous expiration date was already focusable
  // 3 There was no expiration date (and there is a candidate)
  // 4 Previous candidate was not focusable and new candidate is focusable
  // 5 Previous candidate was not focusable and new candidate is not focusable
  if (HasFocusableExpiration() || (!month_candidate && !date_candidate)) {
    // Case 1 and 2
    return false;
  }
  if (!HasExpiration() || (month_candidate && month_candidate->is_focusable) ||
      (year_candidate && year_candidate->is_focusable)) {
    // Case 3 and 4
    expiration_month_ = month_candidate;
    expiration_year_ = year_candidate;
    expiration_date_ = date_candidate;
    return true;
  }
  // Case 5
  return false;
}

bool CreditCardField::HasFocusableExpiration() const {
  if (!expiration_date_ && !expiration_month_) {
    return false;
  }
  if (expiration_date_) {
    return expiration_date_->is_focusable;
  }
  return expiration_month_->is_focusable;
}

bool CreditCardField::ParseFieldSpecificsAllowingReplaceAndSkips(
    AutofillScanner* scanner,
    const base::string16& pattern,
    int match_type,
    int allowed_skips,
    bool allow_replace,
    AutofillField** match) {
  if (*match && !allow_replace)
    return false;
  size_t saved_cursor = scanner->SaveCursor();
  bool found = false;
  AutofillField* candidate = nullptr;
  for (int i = 0; i <= allowed_skips; i++) {
    if (scanner->IsEnd())
      break;
    if (CreditCardField::ParseFieldSpecifics(scanner, pattern, match_type,
                                             &candidate)) {
      if (ConsiderNewCandidate(match, candidate)) {
        found = true;
        if (scanner->IsEnd() || candidate->is_focusable) {
          return true;
        }
        saved_cursor = scanner->SaveCursor();

        continue;
      } else {
        candidate = nullptr;
      }
    }
    if (scanner->Cursor()->is_focusable) {
      break;
    }
    scanner->Advance();
  }
  scanner->RewindTo(saved_cursor);
  return found;
}

}  // namespace autofill
