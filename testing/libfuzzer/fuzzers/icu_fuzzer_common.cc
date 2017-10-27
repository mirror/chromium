// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/libfuzzer/fuzzers/icu_fuzzer_common.h"

#include <iostream>
#include <list>
#include <memory>
#include <regex>

#include "base/logging.h"
#include "unicode/coll.h"
#include "unicode/decimfmt.h"
#include "unicode/dtptngen.h"
#include "unicode/gregocal.h"


namespace icu_fuzzer {

using namespace std;

bool ExtractString(const uint8_t*& data, size_t& size, std::string& str) {
  if (!size)
    return false;

  size_t i = 0;
  for (; i < size; i++) {
    if (!data[i])
      break;
  }
  if (i < size) {
    str.assign(reinterpret_cast<const char*>(data), i);
    size -= i + 1;
    data += i + 1;
  } else {
    str.assign(reinterpret_cast<const char*>(data), size);
    size = 0;
    data = NULL;
  }
  return true;
}

bool ExtractStringSetting(const uint8_t*& data,
                          size_t& size,
                          const char* key,
                          icu::UnicodeString* setting) {
  if (!size)
    return false;

  size_t i = 0;
  for (; i < size; i++) {
    if (!data[i])
      break;
  }
  if (i < size) {
    const char* ptr = reinterpret_cast<const char*>(data);
    size -= i + 1;
    data += i + 1;
    if (!i)
      return false;
    *setting = icu::UnicodeString::fromUTF8(ptr);
    return true;
  } else {
    size = 0;
    data = NULL;
    return false;
  }
}

bool ExtractBooleanSetting(const uint8_t*& data,
                           size_t& size,
                           const char* key,
                           bool* value) {
  if (size >= sizeof(bool)) {
    *value = reinterpret_cast<const bool*>(data)[0];
    data += sizeof(bool);
    size -= sizeof(bool);
    return true;
  }
  return false;
}

bool ExtractIntegerSetting(const uint8_t*& data,
                           size_t& size,
                           const char* key,
                           int32_t* value) {
  bool bExist;
  if (!ExtractBooleanSetting(data, size, key, &bExist) || !bExist)
    return false;
  if (size >= sizeof(int32_t)) {
    *value = reinterpret_cast<const int32_t*>(data)[0];
    data += sizeof(int32_t);
    size -= sizeof(int32_t);
    return true;
  }
  return false;
}

string CanonicalizeLanguageTag(string locale_id) {
  const char* const kInvalidTag = "invalid-tag";
  UErrorCode error = U_ZERO_ERROR;
  char icu_result[ULOC_FULLNAME_CAPACITY];
  int icu_length = 0;

  uloc_forLanguageTag(locale_id.c_str(), icu_result, ULOC_FULLNAME_CAPACITY,
                      &icu_length, &error);
  if (U_FAILURE(error) || error == U_STRING_NOT_TERMINATED_WARNING) {
    return kInvalidTag;
  }

  char result[ULOC_FULLNAME_CAPACITY];

  // Force strict BCP47 rules.
  int32_t result_length = uloc_toLanguageTag(icu_result, result, ULOC_FULLNAME_CAPACITY, TRUE, &error);

  if (U_FAILURE(error) || error == U_STRING_NOT_TERMINATED_WARNING) {
    return kInvalidTag;
  }

  return std::string(result, result_length);
}

icu::SimpleDateFormat* CreateICUDateFormat(const uint8_t*& data,
                                           const icu::Locale& icu_locale,
                                           size_t& size) {
  // Create time zone as specified by the user. We have to re-create time zone
  // since calendar takes ownership.
  icu::TimeZone* tz = NULL;
  std::string stz;
  bool ret = ExtractString(data, size, stz);
  if (ret) {
    icu::UnicodeString timezone = icu::UnicodeString::fromUTF8(stz.c_str());
    tz = icu::TimeZone::createTimeZone(timezone);
  } else {
    tz = icu::TimeZone::createDefault();
  }

  // Create a calendar using locale, and apply time zone to it.
  UErrorCode status = U_ZERO_ERROR;
  icu::Calendar* calendar =
      icu::Calendar::createInstance(tz, icu_locale, status);

  if (calendar->getDynamicClassID() ==
      icu::GregorianCalendar::getStaticClassID()) {
    icu::GregorianCalendar* gc = (icu::GregorianCalendar*)calendar;
    UErrorCode status = U_ZERO_ERROR;
    // The beginning of ECMAScript time, namely -(2**53)
    const double start_of_time = -9007199254740992;
    gc->setGregorianChange(start_of_time, status);
    DCHECK(U_SUCCESS(status));
  }

  // Make formatter from skeleton. Calendar and numbering system are added
  // to the locale as Unicode extension (if they were specified at all).
  icu::SimpleDateFormat* date_format = NULL;
  icu::UnicodeString skeleton;
  icu::UnicodeString allowskel = icu::UnicodeString::fromUTF8("EGyMdjhHmsz");
  if (ExtractStringSetting(data, size, "skeleton", &skeleton)) {
    for (int i = 0; i < skeleton.length(); i++) {
      if (allowskel.indexOf(skeleton[i]) == -1) {
        delete calendar;
        return nullptr;
      }
    }
    std::unique_ptr<icu::DateTimePatternGenerator> generator(
        icu::DateTimePatternGenerator::createInstance(icu_locale, status));
    icu::UnicodeString pattern;
    if (U_SUCCESS(status))
      pattern = generator->getBestPattern(skeleton, status);

    date_format = new icu::SimpleDateFormat(pattern, icu_locale, status);
    if (U_SUCCESS(status)) {
      date_format->adoptCalendar(calendar);
    }
  }

  if (U_FAILURE(status) || !date_format) {
    delete calendar;
    if (date_format) {
      delete date_format;
      date_format = nullptr;
    }
  }

  return date_format;
}

icu::DecimalFormat* CreateICUNumberFormat(const uint8_t*& data,
                                          const icu::Locale& icu_locale,
                                          size_t& size) {
  // Make formatter from size. Numbering system is added
  // to the locale as Unicode extension (if it was specified at all).
  UErrorCode status = U_ZERO_ERROR;
  icu::DecimalFormat* number_format = NULL;
  icu::UnicodeString style;
  icu::UnicodeString currency;
  if (ExtractStringSetting(data, size, "style", &style)) {
    if (style == UNICODE_STRING_SIMPLE("currency")) {
      icu::UnicodeString display;
      ExtractStringSetting(data, size, "currency", &currency);
      ExtractStringSetting(data, size, "currencyDisplay", &display);

#if (U_ICU_VERSION_MAJOR_NUM == 4) && (U_ICU_VERSION_MINOR_NUM <= 6)
      icu::NumberFormat::EStyles format_style;
      if (display == UNICODE_STRING_SIMPLE("code")) {
        format_style = icu::NumberFormat::kIsoCurrencyStyle;
      } else if (display == UNICODE_STRING_SIMPLE("name")) {
        format_style = icu::NumberFormat::kPluralCurrencyStyle;
      } else {
        format_style = icu::NumberFormat::kCurrencyStyle;
      }
#else  // ICU version is 4.8 or above (we ignore versions below 4.0).
      UNumberFormatStyle format_style;
      if (display == UNICODE_STRING_SIMPLE("code")) {
        format_style = UNUM_CURRENCY_ISO;
      } else if (display == UNICODE_STRING_SIMPLE("name")) {
        format_style = UNUM_CURRENCY_PLURAL;
      } else {
        format_style = UNUM_CURRENCY;
      }
#endif

      number_format = static_cast<icu::DecimalFormat*>(
          icu::NumberFormat::createInstance(icu_locale, format_style, status));

      if (U_FAILURE(status)) {
        delete number_format;
        return NULL;
      }
    } else if (style == UNICODE_STRING_SIMPLE("percent")) {
      number_format = static_cast<icu::DecimalFormat*>(
          icu::NumberFormat::createPercentInstance(icu_locale, status));
      if (U_FAILURE(status)) {
        delete number_format;
        return NULL;
      }
      // Make sure 1.1% doesn't go into 2%.
      number_format->setMinimumFractionDigits(1);
    } else {
      // Make a decimal instance by default.
      number_format = static_cast<icu::DecimalFormat*>(
          icu::NumberFormat::createInstance(icu_locale, status));
    }
  } else {
    // Make a decimal instance by default.
    number_format = static_cast<icu::DecimalFormat*>(
        icu::NumberFormat::createInstance(icu_locale, status));
  }

  if (U_FAILURE(status)) {
    delete number_format;
    return NULL;
  }

  // Set all size.

  int32_t digits;
  if (ExtractIntegerSetting(data, size, "minimumIntegerDigits", &digits)) {
    if (digits < 1 || digits > 21)
      digits = 1;
    number_format->setMinimumIntegerDigits(digits);
  }

  if (ExtractIntegerSetting(data, size, "minimumFractionDigits", &digits)) {
    if (digits < 0 || digits > 20)
      digits = 0;
    number_format->setMinimumFractionDigits(digits);
  }

  if (ExtractIntegerSetting(data, size, "maximumFractionDigits", &digits)) {
    if (digits < 0 || digits > 20)
      digits = 20;
    number_format->setMaximumFractionDigits(digits);
  }

  bool significant_digits_used = false;
  int32_t mnsd = 1;
  int32_t mxsd = 21;
  bool bmnsd =
      ExtractIntegerSetting(data, size, "minimumSignificantDigits", &mnsd);
  bool bmxsd =
      ExtractIntegerSetting(data, size, "maximumSignificantDigits", &mxsd);
  if (bmnsd || bmxsd) {
    if (mnsd < 1 || mnsd > 21)
      mnsd = 1;
    if (mxsd < mnsd || mxsd > 21)
      mxsd = 21;
    number_format->setMinimumSignificantDigits(mnsd);
    number_format->setMaximumSignificantDigits(mxsd);
    significant_digits_used = true;
  }

  number_format->setSignificantDigitsUsed(significant_digits_used);

  bool grouping;
  if (ExtractBooleanSetting(data, size, "useGrouping", &grouping)) {
    number_format->setGroupingUsed(grouping);
  }

  // Set rounding mode.
  number_format->setRoundingMode(icu::DecimalFormat::kRoundHalfUp);

  return number_format;
}

icu::Collator* CreateICUCollator(const uint8_t*& data,
                                 const icu::Locale& icu_locale,
                                 size_t& size) {
  // Make collator from size.
  icu::Collator* collator = NULL;
  UErrorCode status = U_ZERO_ERROR;
  collator = icu::Collator::createInstance(icu_locale, status);

  if (U_FAILURE(status)) {
    delete collator;
    return NULL;
  }

  // Set flags first, and then override them with sensitivity if necessary.
  bool numeric;
  if (ExtractBooleanSetting(data, size, "numeric", &numeric)) {
    collator->setAttribute(UCOL_NUMERIC_COLLATION, numeric ? UCOL_ON : UCOL_OFF,
                           status);
  }

  // Normalization is always on, by the spec. We are free to optimize
  // if the strings are already normalized (but we don't have a way to tell
  // that right now).
  collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

  icu::UnicodeString case_first;
  if (ExtractStringSetting(data, size, "caseFirst", &case_first)) {
    if (case_first == UNICODE_STRING_SIMPLE("upper")) {
      collator->setAttribute(UCOL_CASE_FIRST, UCOL_UPPER_FIRST, status);
    } else if (case_first == UNICODE_STRING_SIMPLE("lower")) {
      collator->setAttribute(UCOL_CASE_FIRST, UCOL_LOWER_FIRST, status);
    } else {
      // Default (false/off).
      collator->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    }
  }

  icu::UnicodeString sensitivity;
  if (ExtractStringSetting(data, size, "sensitivity", &sensitivity)) {
    if (sensitivity == UNICODE_STRING_SIMPLE("base")) {
      collator->setStrength(icu::Collator::PRIMARY);
    } else if (sensitivity == UNICODE_STRING_SIMPLE("accent")) {
      collator->setStrength(icu::Collator::SECONDARY);
    } else if (sensitivity == UNICODE_STRING_SIMPLE("case")) {
      collator->setStrength(icu::Collator::PRIMARY);
      collator->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
    } else {
      // variant (default)
      collator->setStrength(icu::Collator::TERTIARY);
    }
  }

  bool ignore;
  if (ExtractBooleanSetting(data, size, "ignorePunctuation", &ignore)) {
    if (ignore) {
      collator->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
    }
  }

  return collator;
}

bool checkTimeZone(icu::SimpleDateFormat* date_format) {
  UErrorCode status = U_ZERO_ERROR;
  // Set time zone and calendar.
  const icu::Calendar* calendar = date_format->getCalendar();
  // getType() returns legacy calendar type name instead of LDML/BCP47 calendar
  // key values. intl.js maps them to BCP47 values for key "ca".
  // TODO(jshin): Consider doing it here, instead.
  // const char* calendar_name = calendar->getType();

  const icu::TimeZone& tz = calendar->getTimeZone();
  icu::UnicodeString time_zone;
  tz.getID(time_zone);

  icu::UnicodeString canonical_time_zone;
  icu::TimeZone::getCanonicalID(time_zone, canonical_time_zone, status);
  if ((U_SUCCESS(status)) &&
      canonical_time_zone != UNICODE_STRING_SIMPLE("Etc/Unknown"))
    return true;
  return false;
}

// static
icu::SimpleDateFormat* DateFormat::InitializeDateTimeFormat(
    const uint8_t*& data,
    const char* locale,
    size_t& size) {
  // Convert BCP47 into ICU locale format.
  UErrorCode status = U_ZERO_ERROR;
  icu::Locale icu_locale;
  char icu_result[ULOC_FULLNAME_CAPACITY];
  int icu_length = 0;
  uloc_forLanguageTag(locale, icu_result, ULOC_FULLNAME_CAPACITY, &icu_length,
                      &status);
  if (U_FAILURE(status) || icu_length == 0) {
    return NULL;
  }
  icu_locale = icu::Locale(icu_result);

  icu::SimpleDateFormat* date_format =
      CreateICUDateFormat(data, icu_locale, size);
  if (!date_format) {
    // Remove extensions and try again.
    icu::Locale no_extension_locale(icu_locale.getBaseName());
    date_format = CreateICUDateFormat(data, no_extension_locale, size);
  }
  if (date_format) {
    if (!checkTimeZone(date_format)) {
      delete date_format;
      date_format = NULL;
    }
  }

  return date_format;
}

icu::DecimalFormat* NumberFormat::InitializeNumberFormat(const uint8_t*& data,
                                                         const char* locale,
                                                         size_t& size) {
  // Convert BCP47 into ICU locale format.
  UErrorCode status = U_ZERO_ERROR;
  icu::Locale icu_locale;
  char icu_result[ULOC_FULLNAME_CAPACITY];
  int icu_length = 0;
  uloc_forLanguageTag(locale, icu_result, ULOC_FULLNAME_CAPACITY, &icu_length,
                      &status);
  if (U_FAILURE(status) || icu_length == 0) {
    return NULL;
  }
  icu_locale = icu::Locale(icu_result);

  icu::DecimalFormat* number_format =
      CreateICUNumberFormat(data, icu_locale, size);
  if (!number_format) {
    // Remove extensions and try again.
    icu::Locale no_extension_locale(icu_locale.getBaseName());
    number_format = CreateICUNumberFormat(data, no_extension_locale, size);
  }

  return number_format;
}

icu::Collator* Collator::InitializeCollator(const uint8_t*& data,
                                            const char* locale,
                                            size_t& size) {
  // Convert BCP47 into ICU locale format.
  UErrorCode status = U_ZERO_ERROR;
  icu::Locale icu_locale;
  char icu_result[ULOC_FULLNAME_CAPACITY];
  int icu_length = 0;
  uloc_forLanguageTag(locale, icu_result, ULOC_FULLNAME_CAPACITY, &icu_length,
                      &status);
  if (U_FAILURE(status) || icu_length == 0) {
    return NULL;
  }
  icu_locale = icu::Locale(icu_result);

  icu::Collator* collator = CreateICUCollator(data, icu_locale, size);
  if (!collator) {
    // Remove extensions and try again.
    icu::Locale no_extension_locale(icu_locale.getBaseName());
    collator = CreateICUCollator(data, no_extension_locale, size);
  }

  return collator;
}

}  // namespace icu_fuzzer
