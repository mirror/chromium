// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/html_based_username_detector.h"

#include "base/i18n/case_conversion.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"

using blink::WebFormControlElement;
using blink::WebInputElement;

namespace autofill {

namespace {

// HTML attributes whose values are analyzed for username identification.
// We either analyze developer group (values used by the developer in the code)
// or user group (values that are visible to the user).
// Developer group is the concatenation of field name and field id.
// User group is obtained with InferLabelForElement function, and considers
// label, placeholder and surrounding context in the DOM.
enum HTMLAttributeGroup { DEVELOPER_GROUP, USER_GROUP };

// For each input element that can be username, we compute and save developer
// and user group, along with associated short tokens lists (to handle finding
// less than 4 letters long words).
struct PossibleUsername {
  WebInputElement input_element;
  base::string16 developer_group;
  base::string16 user_group;
  std::vector<base::string16> short_tokens_in_developer_group;
  std::vector<base::string16> short_tokens_in_user_group;
};

// List of separators that can appear in HTML attribute values.
std::string kDelimiters = "\"\'?%*@!\\/&^#:+~`;,>|<.[](){}-_ 0123456789";

// "Latin" translations are the translations of the words for which the
// original translation is similar to the romanized translation (translation of
// the word only using ISO basic Latin alphabet).
// "Non-latin" translations are the translations of the words that have custom,
// country specific characters.
const char* const kNegativesLatin[] = {
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
constexpr int kNegativesLatinSize = arraysize(kNegativesLatin);

const char* const kNegativesNonLatin[] = {"fjalëkalim",
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
constexpr int kNegativesNonLatinSize = arraysize(kNegativesNonLatin);

const char* const kUsernameLatin[] = {
    "gatti",      "uzantonomo",   "solonanarana",    "nombredeusuario",
    "olumulo",    "nomenusoris",  "enwdefnyddiwr",   "nomdutilisateur",
    "lolowera",   "notandanafn",  "nomedeusuario",   "vartotojovardas",
    "username",   "ahanjirimara", "gebruikersnaam",  "numedeutilizator",
    "brugernavn", "benotzernumm", "jinalamtumiaji",  "erabiltzaileizena",
    "brukernavn", "benutzername", "sunanmaiamfani",  "foydalanuvchinomi",
    "mosebedisi", "kasutajanimi", "ainmcleachdaidh", "igamalomsebenzisi",
    "nomdusuari", "lomsebenzisi", "jenengpanganggo", "ingoakaiwhakamahi",
    "nomeutente", "namapengguna"};
constexpr int kUsernameLatinSize = arraysize(kUsernameLatin);

const char* const kUsernameNonLatin[] = {"用户名",
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
constexpr int kUsernameNonLatinSize = arraysize(kUsernameNonLatin);

const char* const kUserLatin[] = {
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
constexpr int kUserLatinSize = arraysize(kUserLatin);

const char* const kUserNonLatin[] = {"用户",
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
constexpr int kUserNonLatinSize = arraysize(kUserNonLatin);

// Words that certainly point to a username field, if they appear in developer
// group. They are technical words, because they can only be used as variable
// names, and not as stand-alone words.
const char* const kTechnicalWords[] = {
    "uid",         "newtel",     "uaccount",   "regaccount",  "ureg",
    "loginid",     "laddress",   "accountreg", "regid",       "regname",
    "loginname",   "membername", "uname",      "ucreate",     "loginmail",
    "accountname", "umail",      "loginreg",   "accountid",   "loginaccount",
    "ulogin",      "regemail",   "newmobile",  "accountlogin"};
constexpr int kTechnicalWordsSize = arraysize(kTechnicalWords);

// Words that might point to a username field.They have the smallest priority
// in the heuristic, because there are also field names that contain them, but
// are not username fields.
const char* const kWeakWords[] = {"id", "login", "mail"};
constexpr int kWeakWordsSize = arraysize(kWeakWords);

// Compute group and short tokens for developer or user.
void ComputeUsernameGroup(const base::string16& text,
                          const HTMLAttributeGroup& HTML_group,
                          PossibleUsername* username) {
  base::string16 lowercase_text = base::i18n::ToLower(text);
  std::vector<base::StringPiece16> tokens =
      base::SplitStringPiece(lowercase_text, base::ASCIIToUTF16(kDelimiters),
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  base::string16 group = base::JoinString(tokens, base::ASCIIToUTF16(""));

  // Only copy tokens with less than 4 letters.
  std::vector<base::StringPiece16> short_tokens;
  std::copy_if(tokens.begin(), tokens.end(),
               std::inserter(short_tokens, short_tokens.end()),
               [](base::StringPiece16 token) { return token.size() < 4; });

  if (HTML_group == DEVELOPER_GROUP) {
    username->developer_group = group;
    for (base::StringPiece16 token : short_tokens) {
      username->short_tokens_in_developer_group.push_back(
          base::i18n::ToLower(token));
    }
  } else {
    username->user_group = group;
    for (base::StringPiece16 token : short_tokens) {
      username->short_tokens_in_user_group.push_back(
          base::i18n::ToLower(token));
    }
  }
}

// Extracts values of user and developer groups from |WebInputElement|s to
// |PossibleUsername|s.
void MatchFormDataToPossibleUsernames(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    std::vector<PossibleUsername>* possible_usernames) {
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

        // Compute and save groups for possible username.
        PossibleUsername username;
        username.input_element = input_element;
        // Add '$' safety guard when concatenating name and id to prevent
        // forming of new words in between.
        ComputeUsernameGroup(field.name + base::ASCIIToUTF16("$") + field.id,
                             DEVELOPER_GROUP, &username);
        ComputeUsernameGroup(field.label, USER_GROUP, &username);
        possible_usernames->push_back(username);
        break;
      }
    }
  }
}

// Check if a possible username contains any negative word, either in developer
// group, or in user group.
// In developer group only latin negatives can be encountered, and in user
// group both latin and non-latins can be encountered.
bool ContainsNegativeWords(const PossibleUsername& possible_username) {
  // Check for latin negative words.
  for (size_t i = 0; i < kNegativesLatinSize; ++i) {
    if (possible_username.developer_group.find(
            base::UTF8ToUTF16(kNegativesLatin[i])) != std::string::npos ||
        possible_username.user_group.find(
            base::UTF8ToUTF16(kNegativesLatin[i])) != std::string::npos) {
      return true;
    }
  }
  for (size_t i = 0; i < kNegativesNonLatinSize; ++i) {
    if (possible_username.user_group.find(
            base::UTF8ToUTF16(kNegativesNonLatin[i])) != std::string::npos) {
      return true;
    }
  }
  return false;
}

// Remove from |possible_usernames| the elements that definitely cannot be
// usernames.
// If at least one negative word appears in developer or user group, then the
// field is certainly not a username field.
void RemoveFieldsWithNegativeWords(
    std::vector<PossibleUsername>* possible_usernames) {
  possible_usernames->erase(
      std::remove_if(possible_usernames->begin(), possible_usernames->end(),
                     ContainsNegativeWords),
      possible_usernames->end());
}

// Given a form and a dictionary, search if any word from the dictionary
// appears as a substring in the specified group (|group| points either to
// developer, or to user group).
// We iterate over the words from |dictionary| and see how many fields from the
// form have each word as a substring.
// If a word appears in more than 2 fields, we do not make a decision, because
// it may just be a prefix.
// If a word appears in 1 or 2 fields, we return the first field in which we
// found the substring as |username_element|.
bool SearchForPartialMatch(
    const std::vector<PossibleUsername>& possible_usernames,
    const char* const* dictionary,
    const size_t& dictionary_size,
    const HTMLAttributeGroup& group,
    WebInputElement* username_element) {
  // Auxiliary element that contains the first field (in order of appearance in
  // the form) in which a substring is encountered.
  WebInputElement chosen_field;

  for (size_t i = 0; i < dictionary_size; ++i) {
    // Number of fields from the form in which a |dictionary| word appears.
    size_t count = 0;

    // Word from the dictionary is short, so only search in short tokens group.
    if (strlen(dictionary[i]) < 4) {
      for (const PossibleUsername& possible_username : possible_usernames) {
        std::vector<base::string16> short_tokens =
            group == DEVELOPER_GROUP
                ? possible_username.short_tokens_in_developer_group
                : possible_username.short_tokens_in_user_group;
        for (base::string16 token :
             possible_username.short_tokens_in_user_group) {
          if (token == base::UTF8ToUTF16(dictionary[i])) {
            if (count == 0)
              chosen_field = possible_username.input_element;
            count++;
          }
        }
      }
    }
    // Word from the dictionary is long, so search for substring.
    else {
      for (const PossibleUsername& possible_username : possible_usernames) {
        base::string16 analyzed_group = group == DEVELOPER_GROUP
                                            ? possible_username.developer_group
                                            : possible_username.user_group;

        if (analyzed_group.find(base::UTF8ToUTF16(dictionary[i])) !=
            std::string::npos) {
          if (count == 0)
            chosen_field = possible_username.input_element;
          count++;
        }
      }
    }

    if (count && count <= 2) {
      *username_element = chosen_field;
      return true;
    }
  }
  return false;
}

}  // namespace

void GetUsernameFieldBasedOnHtmlAttributes(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    WebInputElement* username_element) {
  DCHECK(username_element);

  std::vector<PossibleUsername> possible_usernames;
  MatchFormDataToPossibleUsernames(all_possible_usernames, form_data,
                                   &possible_usernames);
  RemoveFieldsWithNegativeWords(&possible_usernames);

  // These are the searches performed by the username detector.
  // Order of tests is vital: we search for words in descending order of
  // probability to point to a username field. For each word category, we
  // firstly search in user group, if it makes sense, and then in developer
  // group.
  static struct Heuristic {
    const char* const* dictionary;
    const size_t dictionary_size;
    const HTMLAttributeGroup group;
  } all_heuristics[] = {
      // Search for "username", in developer and user group.
      {kUsernameLatin, kUsernameLatinSize, USER_GROUP},
      {kUsernameNonLatin, kUsernameNonLatinSize, USER_GROUP},
      {kUsernameLatin, kUsernameLatinSize, DEVELOPER_GROUP},
      // Search for "user", in developer and user group.
      {kUserLatin, kUserLatinSize, USER_GROUP},
      {kUserNonLatin, kUserNonLatinSize, USER_GROUP},
      {kUserLatin, kUserLatinSize, DEVELOPER_GROUP},
      // Search for technical words, that can only appear in variable names.
      {kTechnicalWords, kTechnicalWordsSize, DEVELOPER_GROUP},
      // Search for words that might indicate to username, but have a low
      // priority (compared to "user" or "username", for example).
      {kWeakWords, kWeakWordsSize, USER_GROUP},
      {kWeakWords, kWeakWordsSize, DEVELOPER_GROUP}};

  for (const Heuristic& heuristic : all_heuristics) {
    if (SearchForPartialMatch(possible_usernames, heuristic.dictionary,
                              heuristic.dictionary_size, heuristic.group,
                              username_element)) {
      break;
    }
  }
}

}  // namespace autofill
