// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/l10n_util_android.h"

#include <stdint.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "jni/LocalizationUtils_jni.h"
#include "third_party/icu/source/common/unicode/uloc.h"
#include "ui/base/l10n/time_format.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace l10n_util {

int jni_call = 0;

jint GetFirstStrongCharacterDirection(JNIEnv* env,
                                      const JavaParamRef<jclass>& clazz,
                                      const JavaParamRef<jstring>& string) {
  return base::i18n::GetFirstStrongCharacterDirection(
      base::android::ConvertJavaStringToUTF16(env, string));
}

bool IsLayoutRtl() {
  static bool is_layout_rtl_cached = false;
  static bool layout_rtl_cache;

  if (!is_layout_rtl_cached) {
    is_layout_rtl_cached = true;
    JNIEnv* env = base::android::AttachCurrentThread();
    layout_rtl_cache =
        static_cast<bool>(Java_LocalizationUtils_isLayoutRtl(env));
  }

  return layout_rtl_cache;
}

namespace {

const char kSeparator1[] = ",";
const char kSeparator2[] = ";";

static const char* const kTestLanguageList[] = {
    "af",      // Afrikaans
    "am",      // Amharic
    "an",      // Aragonese
    "ar",      // Arabic
    "ast",     // Asturian
    "az",      // Azerbaijani
    "be",      // Belarusian
    "bg",      // Bulgarian
    "bh",      // Bihari
    "bn",      // Bengali
    "br",      // Breton
    "bs",      // Bosnian
    "ca",      // Catalan
    "ckb",     // Kurdish (Arabci),  Sorani
    "co",      // Corsican
    "cs",      // Czech
    "cy",      // Welsh
    "da",      // Danish
    "de",      // German
    "de-AT",   // German (Austria)
    "de-CH",   // German (Switzerland)
    "de-DE",   // German (Germany)
    "de-LI",   // German (Liechtenstein)
    "el",      // Greek
    "en",      // English
    "en-AU",   // English (Australia)
    "en-CA",   // English (Canada)
    "en-GB",   // English (UK)
    "en-IN",   // English (India)
    "en-NZ",   // English (New Zealand)
    "en-US",   // English (US)
    "en-ZA",   // English (South Africa)
    "eo",      // Esperanto
    "es",      // Spanish
    "es-419",  // Spanish (Latin America)
    "es-AR",   // Spanish (Argentina)
    "es-CL",   // Spanish (Chile)
    "es-CO",   // Spanish (Colombia)
    "es-CR",   // Spanish (Costa Rica)
    "es-ES",   // Spanish (Spain)
    "es-HN",   // Spanish (Honduras)
    "es-MX",   // Spanish (Mexico)
    "es-PE",   // Spanish (Peru)
    "es-US",   // Spanish (US)
    "es-UY",   // Spanish (Uruguay)
    "es-VE",   // Spanish (Venezuela)
    "et",      // Estonian
    "eu",      // Basque
    "fa",      // Persian
    "fi",      // Finnish
    "fil",     // Filipino
    "fo",      // Faroese
    "fr",      // French
    "fr-CA",   // French (Canada)
    "fr-CH",   // French (Switzerland)
    "fr-FR",   // French (France)
    "fy",      // Frisian
    "ga",      // Irish
    "gd",      // Scots Gaelic
    "gl",      // Galician
    "gn",      // Guarani
    "gu",      // Gujarati
    "ha",      // Hausa
    "haw",     // Hawaiian
    "he",      // Hebrew
    "hi",      // Hindi
    "hmn",     // Hmong
    "hr",      // Croatian
    "hu",      // Hungarian
    "hy",      // Armenian
    "ia",      // Interlingua
    "id",      // Indonesian
    "is",      // Icelandic
    "it",      // Italian
    "it-CH",   // Italian (Switzerland)
    "it-IT",   // Italian (Italy)
    "ja",      // Japanese
    "jv",      // Javanese
    "ka",      // Georgian
    "kk",      // Kazakh
    "km",      // Cambodian
    "kn",      // Kannada
    "ko",      // Korean
    "ku",      // Kurdish
    "ky",      // Kyrgyz
    "la",      // Latin
    "lb",      // Luxembourgish
    "ln",      // Lingala
    "lo",      // Laothian
    "lt",      // Lithuanian
    "lv",      // Latvian
    "mk",      // Macedonian
    "ml",      // Malayalam
    "mn",      // Mongolian
    "mo",      // Moldavian
    "mr",      // Marathi
    "ms",      // Malay
    "mt",      // Maltese
    "nb",      // Norwegian (Bokmal)
    "ne",      // Nepali
    "nl",      // Dutch
    "nn",      // Norwegian (Nynorsk)
    "no",      // Norwegian
    "oc",      // Occitan
    "om",      // Oromo
    "or",      // Oriya
    "pa",      // Punjabi
    "pl",      // Polish
    "ps",      // Pashto
    "pt",      // Portuguese
    "pt-BR",   // Portuguese (Brazil)
    "pt-PT",   // Portuguese (Portugal)
    "qu",      // Quechua
    "rm",      // Romansh
    "ro",      // Romanian
    "ru",      // Russian
    "sd",      // Sindhi
    "sh",      // Serbo-Croatian
    "si",      // Sinhalese
    "sk",      // Slovak
    "sl",      // Slovenian
    "sm",      // Samoan
    "sn",      // Shona
    "so",      // Somali
    "sq",      // Albanian
    "sr",      // Serbian
    "st",      // Sesotho
    "su",      // Sundanese
    "sv",      // Swedish
    "sw",      // Swahili
    "ta",      // Tamil
    "te",      // Telugu
    "tg",      // Tajik
    "th",      // Thai
    "ti",      // Tigrinya
    "tk",      // Turkmen
    "to",      // Tonga
    "tr",      // Turkish
    "tt",      // Tatar
    "tw",      // Twi
    "ug",      // Uighur
    "uk",      // Ukrainian
    "ur",      // Urdu
    "uz",      // Uzbek
    "vi",      // Vietnamese
    "wa",      // Walloon
    "xh",      // Xhosa
    "yi",      // Yiddish
    "yo",      // Yoruba
    "zu",      // Zulu
};
// Common prototype of ICU uloc_getXXX() functions.
typedef int32_t (*UlocGetComponentFunc)(const char*, char*, int32_t,
                                        UErrorCode*);

std::string GetLocaleComponent(const std::string& locale,
                               UlocGetComponentFunc uloc_func,
                               int32_t max_capacity) {
  std::string result;
  UErrorCode error = U_ZERO_ERROR;
  int32_t actual_length = uloc_func(locale.c_str(),
                                    base::WriteInto(&result, max_capacity),
                                    max_capacity,
                                    &error);
  DCHECK(U_SUCCESS(error));
  DCHECK(actual_length < max_capacity);
  result.resize(actual_length);
  return result;
}

ScopedJavaLocalRef<jobject> NewJavaLocale(
    JNIEnv* env,
    const std::string& locale) {
  // TODO(wangxianzhu): Use new Locale API once Android supports scripts.
  std::string language = GetLocaleComponent(
      locale, uloc_getLanguage, ULOC_LANG_CAPACITY);
  std::string country = GetLocaleComponent(
      locale, uloc_getCountry, ULOC_COUNTRY_CAPACITY);
  std::string variant = GetLocaleComponent(
      locale, uloc_getVariant, ULOC_FULLNAME_CAPACITY);
  // language << ", country: " << country << ", variant: " <<variant;
  jni_call++;
  return Java_LocalizationUtils_getJavaLocale(
      env, base::android::ConvertUTF8ToJavaString(env, language),
      base::android::ConvertUTF8ToJavaString(env, country),
      base::android::ConvertUTF8ToJavaString(env, variant));
}

}  // namespace

base::string16 GetDisplayNameForLocale(const std::string& locale,
                                       const std::string& display_locale) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> java_locale =
      NewJavaLocale(env, locale);
  ScopedJavaLocalRef<jobject> java_display_locale =
      NewJavaLocale(env, display_locale);

  //Logging << "LLNB:" << "Call JNI fetch name for : " << locale;
  ScopedJavaLocalRef<jstring> java_result(
      Java_LocalizationUtils_getDisplayNameForLocale(env, java_locale,
                                                     java_display_locale));
  jni_call++;
  return ConvertJavaStringToUTF16(java_result);
}

base::string16 GetDisplayNamesForLocale(const std::string locales,
                                        const std::string& display_locale) {
  JNIEnv* env = base::android::AttachCurrentThread();

  //Logging << "LLNB:" << "Test Perf Metric:" << display_locale;
  base::TimeTicks start = base::TimeTicks::Now();

  if (locales == "batch") {
    std::string displayLang;
    displayLang += GetLocaleComponent(display_locale, uloc_getLanguage,
                                      ULOC_LANG_CAPACITY);
    displayLang += kSeparator1;
    displayLang += GetLocaleComponent(display_locale, uloc_getCountry,
                                      ULOC_COUNTRY_CAPACITY);
    displayLang += kSeparator1;
    displayLang += GetLocaleComponent(display_locale, uloc_getVariant,
                                      ULOC_FULLNAME_CAPACITY);

    std::string allInOne;
    allInOne.reserve((sizeof(displayLang) + 5) * arraysize(kTestLanguageList));
    for (const char* locale : kTestLanguageList) {
      // language
      allInOne +=
          GetLocaleComponent(locale, uloc_getLanguage, ULOC_LANG_CAPACITY);
      allInOne += kSeparator1;
      // country
      allInOne +=
          GetLocaleComponent(locale, uloc_getCountry, ULOC_COUNTRY_CAPACITY);
      allInOne += kSeparator1;
      // variant
      allInOne +=
          GetLocaleComponent(locale, uloc_getVariant, ULOC_FULLNAME_CAPACITY);
      allInOne += kSeparator2;
    }

    ScopedJavaLocalRef<jstring> java_result(
        Java_LocalizationUtils_getDisplayNamesForLocales(
            env, base::android::ConvertUTF8ToJavaString(env, allInOne),
            base::android::ConvertUTF8ToJavaString(env, displayLang)));
    jni_call++;
    base::TimeTicks end = base::TimeTicks::Now();
    //Logging << "LLNB:" << "Batch Metric End with JNI calls: " << jni_call
    // << ", costInMs:" << (end - start).InMilliseconds();  logging << "LLNB:"
    // << "ALLInONE Call JNI result::" << ConvertJavaStringToUTF16(java_result);
  } else {
    for (const char* a_language : kTestLanguageList) {
      GetDisplayNameForLocale(a_language, display_locale);
    }
    base::TimeTicks end = base::TimeTicks::Now();
    // Logging << "LLNB:" << "Loop Metric: End with JNI calls: " << jni_call
    // << ", costInMs:" << (end - start).InMilliseconds();
  }
  return base::string16();
}

ScopedJavaLocalRef<jstring> GetDurationString(JNIEnv* env,
                                              const JavaParamRef<jclass>& clazz,
                                              jlong timeInMillis) {
  ScopedJavaLocalRef<jstring> jtime_remaining =
      base::android::ConvertUTF16ToJavaString(
          env,
          ui::TimeFormat::Simple(
              ui::TimeFormat::FORMAT_REMAINING, ui::TimeFormat::LENGTH_SHORT,
              base::TimeDelta::FromMilliseconds(timeInMillis)));
  return jtime_remaining;
}

}  // namespace l10n_util
