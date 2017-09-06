// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/html_username_detection_classifier.h"

#include "components/autofill/content/renderer/form_autofill_util.h"

using blink::WebFormControlElement;
using blink::WebInputElement;

namespace autofill {

namespace {

// HTML attributes whose values are analyzed for username identification.
// We either analyze the concatenation of name and id, or the label.
enum Attribute { NameAndId, Label };

// Words that cannot appear in name or id of username field.
// If at least one word from |negativeWords| appears in field name, then
// the field is certainly not a username field.
const char* const negativeWords[] = {
    "firstname", "givenname", "middlename", "lastname", "fullname", "password",
    "fake",      "optional",  "fname",      "lname",    "pw",       "pin"};

// Words that certainly point to a username field, if they appear in name or
// id of field.
// It is not useful to also search for these words in field label text, because
// they are only likely to be used variable names.
const char* const positiveWords[] = {
    "username",    "userlogin",    "userreg",    "userid",     "useraccount",
    "usercreate",  "loginname",    "loginreg",   "loginid",    "loginaccount",
    "loginmail",   "regname",      "regid",      "regaccount", "regemail",
    "accountname", "accountlogin", "accountreg", "accountid",  "uname",
    "ulogin",      "uid",          "ureg",       "uaccount",   "ucreate",
    "umail",       "membername",   "newtel",     "newmobile"};

// All following translations are obtained with Goslate library.
// See https://pythonhosted.org/goslate/ for further information.
// Translations of "username" using latin alphabet.
const char* const usernameTranslationsLatin[] = {"username",
                                                 "ahanjirimara",
                                                 "ainmcleachdaidh",
                                                 "ainmusaideora",
                                                 "anvandarnamn",
                                                 "asunpusuuamnny",
                                                 "benotzernumm",
                                                 "benutzername",
                                                 "bllkedaarhesru",
                                                 "brukernavn",
                                                 "brukersnamme",
                                                 "bybhaarkaariirnaam",
                                                 "chuuephuuaich",
                                                 "emriiperdoruesit",
                                                 "enwdefnyddiwr",
                                                 "erabiltzaileizena",
                                                 "felhasznalonev",
                                                 "foydalanuvchinomi",
                                                 "gatti",
                                                 "gebruikersnaam",
                                                 "igamalomsebenzisi",
                                                 "imiakarystalnika",
                                                 "imiakoristuvacha",
                                                 "imiapolzovatelia",
                                                 "ingoakaiwhakamahi",
                                                 "istifadciadi",
                                                 "jenengpanganggo",
                                                 "jhmoohqnkproepraas",
                                                 "jinalamtumiaji",
                                                 "kasutajanimi",
                                                 "kayttajatunnus",
                                                 "khereglegchiinner",
                                                 "khrnnwm",
                                                 "khsrfkhnm",
                                                 "khtyjwnlw",
                                                 "koldonuuchununaty",
                                                 "korisnichkoime",
                                                 "korisnickoime",
                                                 "kullaniciadi",
                                                 "lietotajvards",
                                                 "lolowera",
                                                 "lomsebenzisi",
                                                 "mosebedisi",
                                                 "namapengguna",
                                                 "navebikarhener",
                                                 "nazwauzytkownika",
                                                 "ngaranpamake",
                                                 "nmkhrbry",
                                                 "nmn",
                                                 "nombredeusuario",
                                                 "nomdusuari",
                                                 "nomdutilisateur",
                                                 "nomedeusuario",
                                                 "nomedeutilizador",
                                                 "nomenusoris",
                                                 "nomeutente",
                                                 "nonitilizate",
                                                 "notandanafn",
                                                 "numedeutilizator",
                                                 "ogtagortsoghianowne",
                                                 "olumulo",
                                                 "onomakhreste",
                                                 "paidalanushyaty",
                                                 "potrebitelskoime",
                                                 "prishiilknaamy",
                                                 "pryogkrtaanaam",
                                                 "pynnnrpeyr",
                                                 "saxeli",
                                                 "sayongjaireum",
                                                 "shmmshtmsh",
                                                 "smlmstkhdm",
                                                 "solonanarana",
                                                 "sunanmaiamfani",
                                                 "syyphuuays",
                                                 "tentruynhap",
                                                 "uporabniskoime",
                                                 "upyeeaaktrnaamn",
                                                 "upyogkrtaanaam",
                                                 "uzantonomo",
                                                 "uzivatelskejmeno",
                                                 "uzivatelskemeno",
                                                 "vaaprkrtaanaav",
                                                 "vartotojovardas",
                                                 "vpraashkrtaanaam",
                                                 "yatathaqaamiseme",
                                                 "yonghuming",
                                                 "yuujrpeeru",
                                                 "yuzaming"};

// Translations of "username" using country-specific alphabet.
const char* const usernameTranslationsForeign[] = {"emriipërdoruesit",
                                                   "የተጠቃሚስም",
                                                   "اسمالمستخدم",
                                                   "օգտագործողիանունը",
                                                   "istifadəçiadı",
                                                   "імякарыстальніка",
                                                   "ব্যবহারকারীরনাম",
                                                   "korisničkoime",
                                                   "потребителскоиме",
                                                   "用户名",
                                                   "用戶名",
                                                   "uživatelskéjméno",
                                                   "käyttäjätunnus",
                                                   "brûkersnamme",
                                                   "სახელი",
                                                   "όνομα χρήστη",
                                                   "વપરાશકર્તાનામ",
                                                   "nonitilizatè",
                                                   "שםמשתמש",
                                                   "उपयोगकर्तानाम",
                                                   "felhasználónév",
                                                   "ainmúsáideora",
                                                   "ユーザ名",
                                                   "ಬಳಕೆದಾರಹೆಸರು",
                                                   "пайдаланушыаты",
                                                   "ឈ្មោះកប្រើប្រាស់",
                                                   "사용자이름",
                                                   "navêbikarhêner",
                                                   "колдонуучунунаты",
                                                   "ຊື່ຜູ້ໃຊ້",
                                                   "lietotājvārds",
                                                   "корисничкоиме",
                                                   "ഉപയോക്തൃനാമം",
                                                   "वापरकर्तानाव",
                                                   "хэрэглэгчийннэр",
                                                   "အသုံးပြုသူအမည်",
                                                   "प्रयोगकर्तानाम",
                                                   "کارننوم",
                                                   "نامکاربری",
                                                   "nazwaużytkownika",
                                                   "nomedeusuário",
                                                   "имяпользователя",
                                                   "корисничкоиме",
                                                   "کاتيجونالو",
                                                   "පරිශීලකනාමය",
                                                   "užívateľskémeno",
                                                   "uporabniškoime",
                                                   "ngaranpamaké",
                                                   "användarnamn",
                                                   "логин",
                                                   "பயனர்பெயர்",
                                                   "యూజర్పేరు",
                                                   "ชื่อผู้ใช้",
                                                   "kullanıcıadı",
                                                   "ім'якористувача",
                                                   "کاصارفکانام",
                                                   "têntruynhập",
                                                   "נאמען"};

// Words that might point to a username field, if they appear in name or id
// of field.
// These words will have the smallest priority in the heuristic, because there
// are also field names that contain them, but are not username fields.
const char* const possibleWords[] = {"user", "login",   "id",
                                     "mail", "account", "phone"};

// Translations of "user" using latin alphabet.
const char* const userTranslationsLatin[] = {
    "user",           "anvandare",
    "asunpusuukiu",   "banytsr",
    "benotzer",       "benutzer",
    "bikaranivan",    "bllkedaar",
    "bruger",         "bruker",
    "bybhaarkaarii",  "cleachdaidh",
    "covneegsiv",     "defnyddiwr",
    "dkhrwnkhykhs",   "doutilizador",
    "erabiltzaile",   "fammi",
    "faoiusaideoir",  "foydalanuvchi",
    "gebruiker",      "gumagamit",
    "hasznalo",       "istifadci",
    "istifoda",       "itilizate",
    "kaiwhakamahi",   "karystalnik",
    "kasutaja",       "kayttaja",
    "khereglegchiin", "khrbr",
    "khrestes",       "koldonuuchu",
    "korisnik",       "korisnikot",
    "koristuvach",    "kullanici",
    "lietotajs",      "lmstml",
    "maiamfani",      "meahoohana",
    "mishtamesh",     "mosebedisi",
    "mpampiasa",      "mushandisi",
    "nguoidung",      "notandi",
    "numake",         "ogtvoghiandznagire",
    "olumulo",        "oru",
    "paidalanushy",   "panganggo",
    "pemakai",        "pengguna",
    "perdorues",      "phuuaichngaan",
    "phuuays",        "polzovatel",
    "potrebitel",     "prishiilk",
    "pryogkrtaa",     "pynnnr",
    "qnkproe",        "sayongja",
    "shesaxeb",       "srf",
    "tagatafaaaoga",  "tathaqaami",
    "umsebenzisi",    "uporabnik",
    "upyeeaaktaav",   "upyogkrtaa",
    "usor",           "usuari",
    "usuario",        "utent",
    "utente",         "utilisateur",
    "utilizador",     "vartotojas",
    "vpraashkrtaa",   "wosuta",
    "yonghu",         "yuujr",
    "yuza",           "ywzr"};

// Translations of "user" using country-specific alphabet.
const char* const userTranslationsForeign[] = {"përdorues",
                                               "ተጠቃሚ",
                                               "المستعمل",
                                               "օգտվողիանձնագիրը",
                                               "istifadəçi",
                                               "карыстальнік",
                                               "ব্যবহারকারী",
                                               "потребител",
                                               "用户",
                                               "用戶",
                                               "uživatel",
                                               "käyttäjä",
                                               "brûker",
                                               "შესახებ",
                                               "χρήστης",
                                               "વપરાશકર્તા",
                                               "itilizatè",
                                               "meahoʻohana",
                                               "מִשׁתַמֵשׁ",
                                               "उपयोगकर्ता",
                                               "használó",
                                               "ọrụ",
                                               "faoiúsáideoir",
                                               "ユーザー",
                                               "ಬಳಕೆದಾರ",
                                               "пайдаланушы",
                                               "អ្នកប្រើ",
                                               "사용자",
                                               "bikaranîvan",
                                               "колдонуучу",
                                               "ຜູ້ໃຊ້",
                                               "lietotājs",
                                               "корисникот",
                                               "ഉപയോക്താവ്",
                                               "वापरकर्ता",
                                               "хэрэглэгчийн",
                                               "အသုံးပြုသူကို",
                                               "प्रयोगकर्ता",
                                               "دکارونکيعکس",
                                               "کاربر",
                                               "użytkownik",
                                               "пользователь",
                                               "tagatafaʻaaogā",
                                               "корисник",
                                               "يوزر",
                                               "පරිශීලක",
                                               "užívateľ",
                                               "användare",
                                               "истифода",
                                               "பயனர்",
                                               "యూజర్",
                                               "ผู้ใช้งาน",
                                               "kullanıcı",
                                               "користувач",
                                               "صارف",
                                               "ngườidùng",
                                               "באַניצער"};

// Translations of "login" using latin alphabet.
const char* const loginTranslationsLatin[] = {"login",
                                              "accesso",
                                              "aetulvnn",
                                              "anmeldung",
                                              "avtorizovatsia",
                                              "banye",
                                              "belepek",
                                              "cuul",
                                              "dangnhap",
                                              "daromadan",
                                              "daxilol",
                                              "denglu",
                                              "dnnhkhydl",
                                              "ee",
                                              "ekhaasuurabb",
                                              "ekhoaasuulabob",
                                              "ensaluti",
                                              "entrar",
                                              "gebaa",
                                              "hiditra",
                                              "htkhbrvt",
                                              "hyrje",
                                              "idhol",
                                              "idnkagmus",
                                              "ingia",
                                              "iniciarsesion",
                                              "iniciarsessio",
                                              "kena",
                                              "kirish",
                                              "kirjaudusisaan",
                                              "kiru",
                                              "kiruu",
                                              "laagin",
                                              "leaagang",
                                              "lebet",
                                              "leeaagi",
                                              "lgin",
                                              "lgn",
                                              "logailisteach",
                                              "logare",
                                              "loggain",
                                              "logginn",
                                              "logirajse",
                                              "logisisse",
                                              "logmasuk",
                                              "logpa",
                                              "longin",
                                              "longinkren",
                                              "lowanimuakaunti",
                                              "masuk",
                                              "mewngofnodi",
                                              "mlebu",
                                              "nevtrekh",
                                              "ngema",
                                              "ngenangemvume",
                                              "oturumac",
                                              "ouvrisesyonan",
                                              "pieslegties",
                                              "prihlasitsa",
                                              "prihlasitse",
                                              "prijavitese",
                                              "prijavitise",
                                              "prisijungti",
                                              "prvesh",
                                              "purtellu",
                                              "rogeuin",
                                              "roguin",
                                              "saioahasi",
                                              "shesvla",
                                              "shiga",
                                              "sidentifier",
                                              "skrainn",
                                              "sulod",
                                              "sundese",
                                              "takiuru",
                                              "tekenaan",
                                              "tsjylldkhwl",
                                              "tsyyknryyn",
                                              "ullnulllai",
                                              "ulogovatise",
                                              "uvaistsi",
                                              "vkhod",
                                              "vpisise",
                                              "vviiti",
                                              "woile",
                                              "wrwd",
                                              "zalogujsie"};

// Translations of "login" using country-specific alphabet.
const char* const loginTranslationsForeign[] = {"ግባ",
                                                "تسجيلالدخول",
                                                "լոգին",
                                                "увайсці",
                                                "লগইন",
                                                "вход",
                                                "iniciarsessió",
                                                "登录",
                                                "登錄",
                                                "přihlásitse",
                                                "logpå",
                                                "kirjaudusisään",
                                                "iniciarsesión",
                                                "შესვლა",
                                                "σύνδεση",
                                                "પ્રવેશ",
                                                "ʻeʻe",
                                                "התחברות",
                                                "लॉगइनकरें",
                                                "belépek",
                                                "skráinn",
                                                "logáilisteach",
                                                "ログイン",
                                                "ಲಾಗಿನ್",
                                                "кіру",
                                                "ចូល",
                                                "로그인",
                                                "кирүү",
                                                "ເຂົ້າສູ່ລະບົບ",
                                                "pieslēgties",
                                                "логирајсе",
                                                "ലോഗിൻ",
                                                "idħol",
                                                "लॉगइन",
                                                "нэвтрэх",
                                                "လော့ဂ်အင်",
                                                "लग-इन",
                                                "دننهکیدل",
                                                "ورود",
                                                "zalogujsię",
                                                "авторизоваться",
                                                "пријавитесе",
                                                "لاگان",
                                                "ඇතුල්වන්න",
                                                "prihlásiťsa",
                                                "vpišise",
                                                "iniciarsesión",
                                                "даромадан",
                                                "உள்நுழை",
                                                "లాగిన్",
                                                "เข้าสู่ระบบ",
                                                "oturumaç",
                                                "ввійти",
                                                "لاگان",
                                                "đăngnhập",
                                                "צייכןאריין"};

// Translations of "id" using latin alphabet.
const char* const idTranslationsLatin[] = {"id",
                                           "aaiddi",
                                           "aaiiddii",
                                           "aayddii",
                                           "aiddi",
                                           "aitti",
                                           "aiungdii",
                                           "ca",
                                           "carnedeidentidad",
                                           "dokumentzasamolichnost",
                                           "hwyshkhsy",
                                           "iaby",
                                           "ichwurde",
                                           "identidade",
                                           "identificacio",
                                           "identifikator",
                                           "identyfikatar",
                                           "idkaart",
                                           "iid",
                                           "iskaznica",
                                           "kamemoa",
                                           "lekhsmgaal",
                                           "mappa",
                                           "mataawaqiyaa",
                                           "rhas",
                                           "shinosai",
                                           "shnsh",
                                           "shyyan",
                                           "sinbunjeung",
                                           "tapatybes",
                                           "tautoteta",
                                           "tvdatzehvt"};

// Translations of "id" using country-specific alphabet.
const char* const idTranslationsForeign[] = {"መታወቂያ",
                                             "هويةشخصية",
                                             "ідэнтыфікатар",
                                             "আইডি",
                                             "документзасамоличност",
                                             "identificació",
                                             "ça",
                                             "ichwürde",
                                             "ταυτότητα",
                                             "આઈડી",
                                             "תְעוּדַתזֶהוּת",
                                             "आईडी",
                                             "ಐಡಿ",
                                             "លេខសម្គាល់",
                                             "신분증",
                                             "tapatybės",
                                             "ഐഡി",
                                             "आयडी",
                                             "အိုင်ဒီ",
                                             "شناسه",
                                             "ябы",
                                             "ид",
                                             "carnédeidentidad",
                                             "шиносаи",
                                             "ஐடி",
                                             "ఐడి",
                                             "รหัส",
                                             "ідентифікатор",
                                             "שייַן"};

// Given a string, remove all non-alpha numerical characters and lower case.
// These changes will be applied to a HTML attribute value, before searching
// for specific words, because in the current heuristic other characters than
// letters don't influence the results.
std::string RemoveNonAlpha(base::string16 str) {
  base::string16 name = base::ToLowerASCII(str);
  std::string newStr = "";
  for (char c : name)
    if (base::IsAsciiLower(c))
      newStr += c;
  return newStr;
}

// Count how many times does a substring appear in a string.
int CountSubstring(const std::string& str, const std::string& sub) {
  if (sub.length() == 0)
    return 0;
  int count = 0;
  for (size_t offset = str.find(sub); offset != std::string::npos;
       offset = str.find(sub, offset + sub.length()))
    ++count;
  return count;
}

// Check if a string (which can be either field label, or field name + id)
// contains any word that certainly does not point to a username field.
bool ContainsNoNegativeWords(std::string str) {
  size_t length = sizeof(negativeWords) / sizeof(char*);
  for (size_t k = 0; k < length; ++k)
    if (str.find(negativeWords[k]) != std::string::npos)
      return false;
  return true;
}

// Given a form and a dictionary, search if any word from the dictionary is
// exactly the same as the specified HTML attribute value (name + id or label).
// If such a field is found, save it in |username_element| and return true.
// Else, return false, so that the search will continue.
bool SearchForFullMatch(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    const char* const* dictionary,
    Attribute attribute,
    WebInputElement& username_element) {
  // Control elements
  size_t last_index = 0;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement control_element = control_elements[i];

    // Form data
    for (size_t j = last_index; j < password_form->form_data.fields.size();
         ++j) {
      const FormFieldData& field = password_form->form_data.fields[j];

      if (!control_element.NameForAutofill().IsEmpty()) {
        // Find matching form data and control elements
        if (field.name == control_element.NameForAutofill().Utf16() ||
            field.id == control_element.NameForAutofill().Utf16()) {
          last_index = j + 1;

          WebInputElement* input_element = ToWebInputElement(&control_element);
          if (!input_element || !input_element->IsEnabled())
            continue;

          bool element_is_invisible =
              !form_util::IsWebElementVisible(*input_element);
          if (!element_is_invisible && input_element->IsTextField()) {
            // Remove non alpha characters from chosen HTML attribute value
            std::string str;
            if (attribute == NameAndId)
              str = RemoveNonAlpha(field.name + field.id);
            else
              str = RemoveNonAlpha(field.label);

            // Search for exact match of word in dictionary.
            size_t length = sizeof(dictionary) / sizeof(char*);

            for (size_t k = 0; k < length; ++k)
              if (str == dictionary[k]) {
                username_element = *input_element;
                return true;
              }
          }
          break;
        }
      }
    }
  }
  return false;
}

// Given a form and a dictionary, search if any word from the dictionary
// appears as a substring in the specified HTML attribute value (name + id
// or label). If a word appears more times in the attribute value, choose
// the field where it appears the most times.
// If such a field is found, save it in |username_element| and return true.
// Else, return false, so that the search will continue.
bool SearchForPartialMatch(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    const char* const* dictionary,
    Attribute attribute,
    WebInputElement& username_element) {
  // Control elements
  size_t last_index = 0;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement control_element = control_elements[i];

    // Form data
    for (size_t j = last_index; j < password_form->form_data.fields.size();
         ++j) {
      const FormFieldData& field = password_form->form_data.fields[j];

      if (!control_element.NameForAutofill().IsEmpty()) {
        // Find matching form data and control elements
        if (field.name == control_element.NameForAutofill().Utf16() ||
            field.id == control_element.NameForAutofill().Utf16()) {
          last_index = j + 1;

          WebInputElement* input_element = ToWebInputElement(&control_element);
          if (!input_element || !input_element->IsEnabled())
            continue;

          bool element_is_invisible =
              !form_util::IsWebElementVisible(*input_element);
          if (!element_is_invisible && input_element->IsTextField()) {
            // Remove non alpha characters from chosen HTML attribute value
            std::string str;
            if (attribute == NameAndId)
              str = RemoveNonAlpha(field.name + field.id);
            else
              str = RemoveNonAlpha(field.label);

            // Search for partial match of word in dictionary.
            // Get word with as many appearances of a positive word as possible.
            int max_occurrences = -1;
            size_t length = sizeof(dictionary) / sizeof(char*);

            for (size_t k = 0; k < length; ++k)
              if (str.find(dictionary[k]) != std::string::npos &&
                  ContainsNoNegativeWords(str) &&
                  CountSubstring(str, dictionary[k]) > max_occurrences) {
                username_element = *input_element;
                max_occurrences = CountSubstring(str, dictionary[k]);
              }
            if (max_occurrences != -1)
              return true;
          }
          break;
        }
      }
    }
  }
  return false;
}

}  // namespace

void HTMLGetUsernameField(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    WebInputElement& username_element) {
  // Do not search for username if a value was already found
  bool username_found = true;
  if (username_element.IsNull())
    username_found = false;

  // 1. Search in field label
  // 1.1. Search for "username", latin and foreign
  if (!username_found)
    username_found = SearchForPartialMatch(control_elements, password_form,
                                           usernameTranslationsLatin, Label,
                                           username_element);
  if (!username_found)
    username_found = SearchForPartialMatch(control_elements, password_form,
                                           usernameTranslationsForeign, Label,
                                           username_element);
  // 1.2. Searh for full words, european plus foreign
  // Search for "user"
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form,
                           userTranslationsLatin, Label, username_element);
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form,
                           userTranslationsForeign, Label, username_element);
  // Search for "login"
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form,
                           loginTranslationsLatin, Label, username_element);
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form,
                           loginTranslationsForeign, Label, username_element);
  // Search for "id"
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form, idTranslationsLatin,
                           Label, username_element);
  if (!username_found)
    username_found =
        SearchForFullMatch(control_elements, password_form,
                           idTranslationsForeign, Label, username_element);

  // 2. Search in field name + id
  // 2.1. Search for username, only latin
  if (!username_found)
    username_found = SearchForPartialMatch(control_elements, password_form,
                                           usernameTranslationsLatin, NameAndId,
                                           username_element);
  // 2.2. Search for positive words
  if (!username_found)
    username_found =
        SearchForPartialMatch(control_elements, password_form, positiveWords,
                              NameAndId, username_element);
  // 2.3. Search for weak terms (that might indicate to username field)
  if (!username_found)
    username_found =
        SearchForPartialMatch(control_elements, password_form, possibleWords,
                              NameAndId, username_element);
}

}  // namespace autofill
