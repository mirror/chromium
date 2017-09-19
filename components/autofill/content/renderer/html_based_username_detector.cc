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
// We either analyze developer group (values used by the developer in the code)
// or user group (values that are visible to the user).
// Developer group is the concatenation of field name and field id, with a "$"
// guard in between. Thus, we prevent identifying fake-username words, that
// might appear because of the concatenation.
// User group is obtained with InferLabelForElement function, and considers
// label, placeholder and surrounding context in the DOM. The function does not
// take into account aria-label.
// It might also be useful to concatenate aria-label to user group, to get
// extra information, but from the research done so far, it did not bring
// extra precision.
enum HTMLAttributeGroup { DEVELOPER_GROUP, USER_GROUP };

// Structure in which information needed by the HTML username detector is being
// kept. For each input element that can be username, developer and user groups
// are computed and saved.
struct PossibleUsername {
  WebInputElement input_element;
  std::string developer_group;
  std::string user_group;
};

// "Latin" translations are the translations of the words for which the
// original translation is similar to the romanized translation (translation of
// the word only using ISO basic Latin alphabet).
// "Non-latin" translations are the translations of the words that have custom,
// country specific characters.

// If at least one word from |kNegativesLatin| or |kNegativesNonLatin| appears
// in field developer or user group, then the field is certainly not a username
// field.
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

// Words that certainly point to a username field, if they appear in developer
// group. They are technical words, because they can only be used as variable
// names, and not as stand-alone words.
const char* const kTechnicalWords[] = {
    "uid",         "newtel",     "uaccount",   "regaccount",  "ureg",
    "loginid",     "laddress",   "accountreg", "regid",       "regname",
    "loginname",   "membername", "uname",      "ucreate",     "loginmail",
    "accountname", "umail",      "loginreg",   "accountid",   "loginaccount",
    "ulogin",      "regemail",   "newmobile",  "accountlogin"};

// Words that might point to a username field.They have the smallest priority
// in the heuristic, because there are also field names that contain them, but
// are not username fields.
// As improvement, "id" might also be useful, but it needs a special treatment,
// because it is a short word, and cannot be trusted (maybe surround it with
// guards etc).
const char* const kWeakWords[] = {"login", "mail", "@"};

// Following functions are helper functions for the detector.

// Given an ASCII string:
// - convert it into lowercase
// - only keep letters ([a-z])
// - do not remove "$", because this character is used as a guard.
// These changes will be applied to a developer group, before searching for
// specific words, because in the current heuristic other characters than
// letters don't influence the results.
// The function cannot also be applied to a user group, because field label may
// also contain non-ASCII characters.
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
// These changes will be applied to user group, which may also contain country
// specific characters.
std::string OnlyKeepLettersInNonASCII(const base::string16& str) {
  std::string converted_str = base::UTF16ToUTF8(str);
  std::string new_str;
  for (char c : converted_str) {
    if (base::IsAsciiAlpha(c))
      new_str += base::ToLowerASCII(c);
    else if (!base::IsAsciiWhitespace(c) && !base::IsAsciiDigit(c))
      new_str += c;
  }
  return new_str;
}

// Given a field, return developer group, i.e. concatenation of field name and
// id, with safety guard ('$') in between, and with special characters removed.
std::string GetDeveloperGroup(const FormFieldData& field) {
  std::string guard = "$";
  return OnlyKeepLettersInASCII(field.name + base::ASCIIToUTF16(guard) +
                                field.id);
}

// Given a field, return user group. Only keep ASCII letters and non-latin
// letters, and no other special characters or digits.
std::string GetUserGroup(const FormFieldData& field) {
  return OnlyKeepLettersInNonASCII(field.label);
}

// Out of all possible usernames, exclude elements that have empty developer
// group (because the detector won't have what to analyze on those fields).
// Match the remaining ones to form data needed by the algorithm, i.e.
// field name, id and label, and compute developer and user group.
void MatchFormDataToPossibleUsernames(
    const std::vector<blink::WebInputElement>& all_possible_usernames,
    const FormData& form_data,
    std::vector<PossibleUsername>& possible_usernames) {
  // Web input elements and form data elements are listed in the same order.
  // Thus, we can remember the index of the last matched form data element, so
  // that when we match the next one, we continue to iterate from where we
  // previously stopped.
  size_t current_index = 0;

  for (blink::WebInputElement possible_username : all_possible_usernames) {
    for (size_t i = current_index; i < form_data.fields.size(); ++i) {
      const FormFieldData& field = form_data.fields[i];

      if (!possible_username.NameForAutofill().IsEmpty()) {
        // Find matching form data and web input element.
        if (field.name == possible_username.NameForAutofill().Utf16()) {
          current_index = i + 1;
          // Save needed information.
          PossibleUsername username;
          username.input_element = possible_username;
          username.developer_group = GetDeveloperGroup(field);
          username.user_group = GetUserGroup(field);
          possible_usernames.push_back(username);
          break;
        }
      }
    }
  }
}

// Helper function to remove an element from a vector of PossibleUsername
// elements, if the element has |neg| as a substring in either developer or
// user group.
void RemoveNonUsername(std::vector<PossibleUsername>& pu, std::string neg) {
  pu.erase(std::remove_if(pu.begin(), pu.end(),
                          [&](PossibleUsername const& possible_username) {
                            return (possible_username.developer_group.find(
                                        neg) != std::string::npos ||
                                    possible_username.user_group.find(neg) !=
                                        std::string::npos);
                          }),
           pu.end());
}

// Remove from |possible_usernames| the ones that definitely cannot be
// usernames. This happens when a group (developer or user) contains, as
// substring, at least a negative word (user groups can have latin and
// non-latin negative words, while developer groups can only have latin
// negative words).
void RemoveImpossibleUsernames(
    std::vector<PossibleUsername>& possible_usernames) {
  // When we find latin negative word, we don't also search for non-latin
  // negative word.
  bool negative_found = false;
  for (PossibleUsername possible_username : possible_usernames) {
    // Check for latin negative words.
    for (size_t i = 0; i < arraysize(kNegativesLatin); ++i) {
      if (possible_username.developer_group.find(kNegativesLatin[i]) !=
              std::string::npos ||
          possible_username.user_group.find(kNegativesLatin[i]) !=
              std::string::npos) {
        RemoveNonUsername(possible_usernames, kNegativesLatin[i]);
        negative_found = true;
        break;
      }
    }
    // Check for non-latin negative words.
    if (!negative_found) {
      for (size_t i = 0; i < arraysize(kNegativesNonLatin); ++i) {
        if (possible_username.user_group.find(kNegativesNonLatin[i]) !=
            std::string::npos) {
          RemoveNonUsername(possible_usernames, kNegativesNonLatin[i]);
          break;
        }
      }
    }
  }
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
  // Number of fields from the form in which a |dictionary| word appears.
  size_t count;

  for (size_t i = 0; i < dictionary_size; ++i) {
    count = 0;
    for (PossibleUsername possible_username : possible_usernames) {
      std::string analyzed_group;
      (group == DEVELOPER_GROUP)
          ? analyzed_group = possible_username.developer_group
          : analyzed_group = possible_username.user_group;

      if (analyzed_group.find(dictionary[i]) != std::string::npos) {
        if (!count)
          chosen_field = possible_username.input_element;
        count++;
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
  DCHECK(username_element->IsNull());

  std::vector<PossibleUsername> possible_usernames;
  MatchFormDataToPossibleUsernames(all_possible_usernames, form_data,
                                   possible_usernames);
  RemoveImpossibleUsernames(possible_usernames);

  // These are the searches performed by the username detector.
  // Order of tests is vital: we search for words in descending order of
  // probability to point to a username field. For each word category, we
  // firstly search in user group, if it makes sense, and then in developer
  // group.
  struct Heuristic {
    const char* const* dictionary;
    const size_t dictionary_size;
    const HTMLAttributeGroup group;
  } all_heuristics[] = {
      // Search for "username", in developer and user group.
      {kUsernameLatin, arraysize(kUsernameLatin), USER_GROUP},
      {kUsernameNonLatin, arraysize(kUsernameNonLatin), USER_GROUP},
      {kUsernameLatin, arraysize(kUsernameLatin), DEVELOPER_GROUP},
      // Search for "user", in developer and user group.
      {kUserLatin, arraysize(kUserLatin), USER_GROUP},
      {kUserNonLatin, arraysize(kUserNonLatin), USER_GROUP},
      {kUserLatin, arraysize(kUserLatin), DEVELOPER_GROUP},
      // Search for technical words, that can only appear in variable names.
      {kTechnicalWords, arraysize(kTechnicalWords), DEVELOPER_GROUP},
      // Search for words that might indicate to username, but have a low
      // priority (compared to "user" or "username", for example).
      {kWeakWords, arraysize(kWeakWords), USER_GROUP},
      {kWeakWords, arraysize(kWeakWords), DEVELOPER_GROUP}};

  for (Heuristic heuristic : all_heuristics) {
    if (SearchForPartialMatch(possible_usernames, heuristic.dictionary,
                              heuristic.dictionary_size, heuristic.group,
                              username_element)) {
      break;
    }
  }
}

}  // namespace autofill
