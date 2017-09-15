// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/html_based_username_detector.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"

using blink::WebFormControlElement;
using blink::WebInputElement;

namespace autofill {

namespace {

// HTML attributes whose values are analyzed for username identification.
// We either analyze name group or label group.
// Name group is the concatenation of field name and field id, with a "$"
// guard inbetween. Thus, we prevent identifying short (less than 3 letters)
// words that are not actually in field name or label, but just appear because
// of the concatenation.
// Label group is obtained with InferLabelForElement function, and considers
// label, placeholder and surrounding context in the DOM.
// TODO (anacosmina): justify why we do/do not use aria-label.
enum HTMLAttribute { NAME_GROUP, LABEL_GROUP };

// All following translations are obtained with Goslate library.
// See https://pythonhosted.org/goslate/ for further information.

// List of languages in which we translate:
// Afrikaans, Albanian, Amharic, Arabic, Armenian, Azeerbaijani, Basque,
// Belarusian, Bengali, Bosnian, Bulgarian, Catalan, Cebuano, Chichewa, Chinese
// (Simplified), Chinese (Traditional), Corsican, Croatian, Czech, Danish,
// Dutch, English, Esperanto, Estonian, Filipino, Finnish, French,
// Frisian,Galician, Georgian, German, Greek, Gujarati, Haitian Creole, Hausa,
// Hawaiian, Hebrew, Hindi, Hmong, Hungarian, Icelandic, Igbo, Indonesian,
// Irish, Italian, Japanese, Javanese, Kannada, Kazakh, Khmer, Korean, Kurdish,
// Kyrgyz, Lao, Latin, Latvian, Lithuanian, Luxembourgish, Macedonian, Malagasy,
// Malay, Malayalam, Maltese, Maori, Marathi, Mongolian, Burmese, Nepali,
// Norwegian, Pashto, Persian, Polish, Portuguese, Punjabi, Romanian, Russian,
// Samoan, Scots Gaelic, Serbian, Sesotho, Shona, Sindhi, Sinhala, Slovak,
// Slovenian, Somali, Spanish, Sundanese, Swahili, Swedish, Tajik, Tamil,
// Telugu, Thai, Turkish, Ukrainian, Urdu, Uzbek, Vietnamese, Welsh, Xhosa,
// Yiddish, Yoruba, Zulu.

// "Latin" translations are the translations of the words for which the original
// translation is similar to the romanized translation (translation of the word
// only using ISO basic Latin alphabet).
// "Foreign" translations are the translations of the words that have
// custom, country specific characters.

// Words that cannot appear in name or label group.
// If at least one word from |negativesLatin| or |negativesForeign| appears
// in field name, then the field is certainly not a username field.
const char* const negativesLatin[] = {
    "pin",    "parola",   "wagwoord",   "wachtwoord",
    "fake",   "parole",   "givenname",  "achinsinsi",
    "token",  "parool",   "firstname",  "facalfaire",
    "fname",  "lozinka",  "pasahitza",  "focalfaire",
    "lname",  "passord",  "pasiwedhi",  "iphasiwedi",
    "geslo",  "huahuna",  "passwuert",  "katalaluan",
    "heslo",  "fullname", "phasewete",  "adgangskode",
    "parol",  "optional", "wachtwurd",  "contrasenya",
    "sandi",  "lastname", "cyfrinair",  "contrasinal",
    "senha",  "kupuhipa", "katasandi",  "kalmarsirri",
    "hidden", "password", "loluszais",  "tenimiafina",
    "second", "passwort", "middlename", "paroladordine",
    "codice", "pasvorto", "familyname", "inomboloyokuvula",
    "modpas", "salasana", "motdepasse", "numeraeleiloaesesi"};

const char* const negativesForeign[] = {"fjalëkalim",
                                        "የይለፍቃል",
                                        "كلمهالسر",
                                        "գաղտնաբառ",
                                        "пароль",
                                        "পাসওয়ার্ড",
                                        "парола",
                                        "密码",
                                        "密碼",
                                        "დაგავიწყდათ",
                                        "κωδικόςπρόσβασης",
                                        "પાસવર્ડ",
                                        "סיסמה",
                                        "पासवर्ड",
                                        "jelszó",
                                        "lykilorð",
                                        "paswọọdụ",
                                        "パスワード",
                                        "ಪಾಸ್ವರ್ಡ್",
                                        "пароль",
                                        "ការពាក្យសម្ងាត់",
                                        "암호",
                                        "şîfre",
                                        "купуясөз",
                                        "ລະຫັດຜ່ານ",
                                        "slaptažodis",
                                        "лозинка",
                                        "पासवर्ड",
                                        "нууцүг",
                                        "စကားဝှက်ကို",
                                        "पासवर्ड",
                                        "رمز",
                                        "کلمهعبور",
                                        "hasło",
                                        "пароль",
                                        "лозинка",
                                        "پاسورڊ",
                                        "මුරපදය",
                                        "contraseña",
                                        "lösenord",
                                        "гузарвожа",
                                        "கடவுச்சொல்",
                                        "పాస్వర్డ్",
                                        "รหัสผ่าน",
                                        "пароль",
                                        "پاسورڈ",
                                        "mậtkhẩu",
                                        "פּאַראָל",
                                        "ọrọigbaniwọle"};

const char* const usernameLatin[] = {
    "gatti",      "uzantonomo",   "solonanarana",    "nombredeusuario",
    "olumulo",    "nomenusoris",  "enwdefnyddiwr",   "nomdutilisateur",
    "lolowera",   "notandanafn",  "nomedeusuario",   "vartotojovardas",
    "username",   "ahanjirimara", "gebruikersnaam",  "numedeutilizator",
    "brugernavn", "benotzernumm", "jinalamtumiaji",  "erabiltzaileizena",
    "brukernavn", "benutzername", "sunanmaiamfani",  "foydalanuvchinomi",
    "mosebedisi", "kasutajanimi", "ainmcleachdaidh", "igamalomsebenzisi",
    "nomdusuari", "lomsebenzisi", "jenengpanganggo", "ingoakaiwhakamahi",
    "nomeutente", "namapengguna"};

const char* const usernameForeign[] = {"用户名",
                                       "کاتيجونالو",
                                       "用戶名",
                                       "የተጠቃሚስም",
                                       "логин",
                                       "اسمالمستخدم",
                                       "נאמען",
                                       "کاصارفکانام",
                                       "ユーザ名",
                                       "όνομα χρήστη",
                                       "brûkersnamme",
                                       "корисничкоиме",
                                       "nonitilizatè",
                                       "корисничкоиме",
                                       "ngaranpamaké",
                                       "ຊື່ຜູ້ໃຊ້",
                                       "användarnamn",
                                       "యూజర్పేరు",
                                       "korisničkoime",
                                       "пайдаланушыаты",
                                       "שםמשתמש",
                                       "ім'якористувача",
                                       "کارننوم",
                                       "хэрэглэгчийннэр",
                                       "nomedeusuário",
                                       "имяпользователя",
                                       "têntruynhập",
                                       "பயனர்பெயர்",
                                       "ainmúsáideora",
                                       "ชื่อผู้ใช้",
                                       "사용자이름",
                                       "імякарыстальніка",
                                       "lietotājvārds",
                                       "потребителскоиме",
                                       "uporabniškoime",
                                       "колдонуучунунаты",
                                       "kullanıcıadı",
                                       "පරිශීලකනාමය",
                                       "istifadəçiadı",
                                       "օգտագործողիանունը",
                                       "navêbikarhêner",
                                       "ಬಳಕೆದಾರಹೆಸರು",
                                       "emriipërdoruesit",
                                       "वापरकर्तानाव",
                                       "käyttäjätunnus",
                                       "વપરાશકર્તાનામ",
                                       "felhasználónév",
                                       "उपयोगकर्तानाम",
                                       "nazwaużytkownika",
                                       "ഉപയോക്തൃനാമം",
                                       "სახელი",
                                       "အသုံးပြုသူအမည်",
                                       "نامکاربری",
                                       "प्रयोगकर्तानाम",
                                       "uživatelskéjméno",
                                       "ব্যবহারকারীরনাম",
                                       "užívateľskémeno",
                                       "ឈ្មោះអ្នកប្រើប្រាស់"};

const char* const userLatin[] = {
    "user",   "wosuta",   "gebruiker",  "utilizator",
    "usor",   "notandi",  "gumagamit",  "vartotojas",
    "fammi",  "olumulo",  "maiamfani",  "cleachdaidh",
    "utent",  "pemakai",  "mpampiasa",  "umsebenzisi",
    "bruger", "usuario",  "panganggo",  "utilisateur",
    "bruker", "benotzer", "uporabnik",  "doutilizador",
    "numake", "benutzer", "covneegsiv", "erabiltzaile",
    "usuari", "kasutaja", "defnyddiwr", "kaiwhakamahi",
    "utente", "korisnik", "mosebedisi", "foydalanuvchi",
    "uzanto", "pengguna", "mushandisi"};

const char* const userForeign[] = {"用户",
                                   "użytkownik",
                                   "tagatafaʻaaogā",
                                   "دکارونکيعکس",
                                   "用戶",
                                   "užívateľ",
                                   "корисник",
                                   "карыстальнік",
                                   "brûker",
                                   "kullanıcı",
                                   "истифода",
                                   "អ្នកប្រើ",
                                   "ọrụ",
                                   "ተጠቃሚ",
                                   "באַניצער",
                                   "хэрэглэгчийн",
                                   "يوزر",
                                   "istifadəçi",
                                   "ຜູ້ໃຊ້",
                                   "пользователь",
                                   "صارف",
                                   "meahoʻohana",
                                   "потребител",
                                   "वापरकर्ता",
                                   "uživatel",
                                   "ユーザー",
                                   "מִשׁתַמֵשׁ",
                                   "ผู้ใช้งาน",
                                   "사용자",
                                   "bikaranîvan",
                                   "колдонуучу",
                                   "વપરાશકર્તા",
                                   "përdorues",
                                   "ngườidùng",
                                   "корисникот",
                                   "उपयोगकर्ता",
                                   "itilizatè",
                                   "χρήστης",
                                   "користувач",
                                   "օգտվողիանձնագիրը",
                                   "használó",
                                   "faoiúsáideoir",
                                   "შესახებ",
                                   "ব্যবহারকারী",
                                   "lietotājs",
                                   "பயனர்",
                                   "ಬಳಕೆದಾರ",
                                   "ഉപയോക്താവ്",
                                   "کاربر",
                                   "యూజర్",
                                   "පරිශීලක",
                                   "प्रयोगकर्ता",
                                   "användare",
                                   "المستعمل",
                                   "пайдаланушы",
                                   "အသုံးပြုသူကို",
                                   "käyttäjä"};

// Words that certainly point to a username field, if they appear in name
// group. They are technical words, because they can only be used as
// variable names, and not as standalone words. We look for these words
// only in the name group.
const char* const technicalWords[] = {
    "uid",         "newtel",     "uaccount",   "regaccount",  "ureg",
    "loginid",     "laddress",   "accountreg", "regid",       "regname",
    "loginname",   "membername", "uname",      "ucreate",     "loginmail",
    "accountname", "umail",      "loginreg",   "accountid",   "loginaccount",
    "ulogin",      "regemail",   "newmobile",  "accountlogin"};

// Words that might point to a username field, if they appear in name or id
// of field.
// These words will have the smallest priority in the heuristic, because there
// are also field names that contain them, but are not username fields.
// "id" appears with a "$" guard, because it is a very short word, and thus
// special characters removal might yield to wrong results.
// Ex: "ui_data" would yield to "uidata", after normal removal. If we add guard
// when removing a special character, we get "ui$data", which does not contain
// "$id".
const char* const weakWords[] = {"$id", "login", "mail", "@"};

// Following functions are helper functions for the detector.

// Given an ASCII string:
// - convert it into lowercase
// - only keep letters ([a-z])
// - do not remove "$", because this character is used as a guard.
// These changes will be applied to a name group, before searching for specific
// words, because in the current heuristic other characters than letters don't
// influence the results.
// The function cannot also be applied to a label group, because field label
// may also contain non-ASCII characters.
std::string OnlyKeepLettersInASCII(const base::string16& str) {
  std::string new_str;
  for (char c : str) {
    if (base::IsAsciiAlpha(c) || c == '$')
      new_str += base::ToLowerASCII(c);
  }
  return new_str;
}

// Given a string which also has non-ASCII characters:
// - for each ASCII character: convert it into lowercase and keep it if it is a
// letter ([a-z]).
// - for each non-ASCII character: keep it.
// - TODO (anacosmina): if we encounter " id", replace it with "$id".
// These changes will be applied to field label, which may also contain country
// specific characters.
std::string OnlyKeepLettersInNonASCII(const base::string16& str) {
  std::string new_str;
  for (char c : str) {
    if (base::IsAsciiAlpha(c))
      new_str += base::ToLowerASCII(c);
    else if (!base::IsAsciiWhitespace(c) && !base::IsAsciiDigit(c))
      new_str += c;
  }
  return new_str;
}

// Check if a string (which can name group or label group) is definitely not
// username group. This happens when:
// - field is hidden or disabled
// - field is not text field
// - it contains, as substring, a negative word (label groups can have latin
// and foreign negative words, while name groups can only have latin negative
// words).
void MarkNegatives(
    const std::vector<blink::WebFormControlElement>& control_elements,
    const FormData& form_data,
    std::vector<bool> negatives) {
  size_t last_index = 0;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement control_element = control_elements[i];

    for (size_t j = last_index; j < form_data.fields.size(); ++j) {
      const FormFieldData& field = form_data.fields[j];

      if (!control_element.NameForAutofill().IsEmpty()) {
        // Find matching form data and control elements.
        if (field.name == control_element.NameForAutofill().Utf16() ||
            field.id == control_element.NameForAutofill().Utf16()) {
          last_index = j + 1;

          WebInputElement* input_element = ToWebInputElement(&control_element);
          if (!input_element || !input_element->IsEnabled())
            continue;

          bool element_is_invisible =
              !form_util::IsWebElementVisible(*input_element);
          if (element_is_invisible || !input_element->IsTextField()) {
            // Field is negative if it is hidden or is not text field.
            negatives[j] = true;
          } else {
            // Field is also negative if we encounter any negative word in
            // name group or label group.
            std::string name_group;
            std::string label_group;
            // TODO (anacosmina): add "$" between name and id.
            name_group = OnlyKeepLettersInASCII(field.name + field.id);
            label_group = OnlyKeepLettersInNonASCII(field.label);

            // Check for latin negative words in name and group label.
            for (size_t k = 0; k < arraysize(negativesLatin); ++k) {
              if (name_group.find(negativesLatin[k]) != std::string::npos ||
                  label_group.find(negativesLatin[k]) != std::string::npos) {
                negatives[j] = true;
                break;
              }
            }
            // If no negative has been found yet, also check for foreign
            // negatives, only in label group.
            if (!negatives[j]) {
              for (size_t k = 0; k < arraysize(negativesForeign); ++k) {
                if (label_group.find(negativesForeign[k]) !=
                    std::string::npos) {
                  negatives[j] = true;
                  break;
                }
              }
            }
          }
          break;
        }
      }
    }
  }
}

// Given a form and a dictionary, search if any word from the dictionary
// appears as a substring in the specified group (|attribute| points either to
// name, or to label group).
// Exclude from the beginning the fields that cannot be username.
// At each step, we iterate over the words from |dictionary| and see in how
// many fields from the form have each word as a substring.
// If a word appears in more than 2 fields, we do not make a decision,
// because it may just be a prefix.
// If a word appears in 1 or 2 fields, we return the first field in which we
// found the substring as |username_element|.
bool SearchForPartialMatch(
    const std::vector<blink::WebFormControlElement>& control_elements,
    const FormData& form_data,
    const char* const* dictionary,
    const HTMLAttribute& attribute,
    std::vector<bool> negatives,
    WebInputElement* username_element) {
  // Auxiliary element that contains the first field (in order of appearance in
  // the form) in which a substring is encountered.
  WebInputElement* chosen_field;
  // Number of fields from the form in which a |dictionary| word appears.
  size_t count;

  size_t length = sizeof(dictionary) / sizeof(char*);
  for (size_t k = 0; k < length; ++k) {
    count = 0;
    size_t last_index = 0;
    for (size_t i = 0; i < control_elements.size(); ++i) {
      WebFormControlElement control_element = control_elements[i];

      for (size_t j = last_index; j < form_data.fields.size(); ++j) {
        const FormFieldData& field = form_data.fields[j];

        if (!control_element.NameForAutofill().IsEmpty()) {
          // Find matching form data and control elements.
          if (field.name == control_element.NameForAutofill().Utf16() ||
              field.id == control_element.NameForAutofill().Utf16()) {
            last_index = j + 1;

            // Only take into acccount fields that can be username.
            if (!negatives[j]) {
              WebInputElement* input_element =
                  ToWebInputElement(&control_element);
              if (!input_element || !input_element->IsEnabled())
                continue;

              // Remove non alpha characters from chosen HTML attribute value.
              std::string str;
              // TODO (anacosmina): add "$" between name and id.
              (attribute == NAME_GROUP)
                  ? str = OnlyKeepLettersInASCII(field.name + field.id)
                  : str = OnlyKeepLettersInNonASCII(field.label);

              if (str.find(dictionary[k]) != std::string::npos) {
                if (!count)
                  chosen_field = input_element;
                count++;
              }
            }
          }
        }
      }
    }
    if (count <= 2 && count > 0) {
      username_element = chosen_field;
      return true;
    }
  }
  return false;
}

}  // namespace

void GetUsernameFieldBasedOnHtmlAttributes(
    const std::vector<blink::WebFormControlElement>& control_elements,
    const FormData& form_data,
    WebInputElement* username_element) {
  // Mark fields that cannot be username.
  std::vector<bool> negatives(form_data.fields.size(), false);
  MarkNegatives(control_elements, form_data, negatives);

  // Searches that the username detector performs.
  // Order of tests is vital: we search for words in descending order of
  // probability to point to a username field. For each word category, we
  // firstly search in label group, if it makes sense, and then in name group.
  std::pair<const char* const*, HTMLAttribute> all_heuristics[9] = {
      // Search for "username", in name and label group.
      std::make_pair(usernameLatin, LABEL_GROUP),
      std::make_pair(usernameForeign, LABEL_GROUP),
      std::make_pair(usernameLatin, NAME_GROUP),
      // Search for "user", in name and label group.
      std::make_pair(userLatin, LABEL_GROUP),
      std::make_pair(userForeign, LABEL_GROUP),
      std::make_pair(userLatin, NAME_GROUP),
      // Search for technical words, that can only appear in variable names.
      std::make_pair(technicalWords, NAME_GROUP),
      // Search for words that might indicate to username, but have a very low
      // priority (compared to "user" or "username", for example).
      std::make_pair(weakWords, LABEL_GROUP),
      std::make_pair(weakWords, NAME_GROUP)};

  for (std::pair<const char* const*, HTMLAttribute> heuristic :
       all_heuristics) {
    if (SearchForPartialMatch(control_elements, form_data, heuristic.first,
                              heuristic.second, negatives, username_element)) {
      break;
    }
  }
}

}  // namespace autofill
