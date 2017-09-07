// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/html_username_detection_classifier.h"

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"

using blink::WebFormControlElement;
using blink::WebInputElement;

namespace autofill {

namespace {

// HTML attributes whose values are analyzed for username identification.
// We either analyze the concatenation of name and id, or the label.
enum HTMLAttribute { NAME_AND_ID, LABEL };

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

// Words that might point to a username field, if they appear in name or id
// of field.
// These words will have the smallest priority in the heuristic, because there
// are also field names that contain them, but are not username fields.
const char* const possibleWords[] = {"user", "login",   "id",
                                     "mail", "account", "phone"};

// All following translations are obtained with Goslate library.
// Translations are romanized, i.e. they all use ISO basic Latin alphabet.
// See https://pythonhosted.org/goslate/ for further information.
const char* const usernameTranslations[] = {"username",
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

const char* const userTranslations[] = {"user",           "anvandare",
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

const char* const loginTranslations[] = {"login",
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

const char* const idTranslations[] = {"id",
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

// Given an ASCII string, convert it into lowercase and only keep letters
// ([a-z]).
// These changes will be applied to a field name + id value, before
// searching for specific words, because in the current heuristic other
// characters than letters don't influence the results.
// The function cannot also be applied on field label, because field label
// may also contain non-ASCII characters.
std::string OnlyKeepLettersInASCII(const base::string16& str) {
  std::string new_str;
  for (char c : str) {
    if (base::IsAsciiAlpha(c))
      new_str += base::ToLowerASCII(c);
  }
  return new_str;
}

// Given a string:
// - for each ASCII character: convert it into lowercase and keep it if it is a
// letter ([a-z]).
// - for each non-ASCII character: keep it.
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

// Count how many times a substring appears (non-overlapping) in a string.
size_t CountSubstring(const std::string& str, const std::string& sub) {
  if (!sub.length())
    return 0;
  size_t count = 0;
  for (size_t offset = str.find(sub); offset != std::string::npos;
       offset = str.find(sub, offset + sub.length())) {
    ++count;
  }
  return count;
}

// Check if a string (which can be either field label, or field name + id)
// contains any word that certainly points to a non-username field.
bool ContainsNoNegativeWords(const std::string& str) {
  for (size_t k = 0; k < arraysize(negativeWords); ++k) {
    if (str.find(negativeWords[k]) != std::string::npos)
      return false;
  }
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
    HTMLAttribute attribute,
    WebInputElement& username_element) {
  size_t last_index = 0;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement control_element = control_elements[i];

    for (size_t j = last_index; j < password_form->form_data.fields.size();
         ++j) {
      const FormFieldData& field = password_form->form_data.fields[j];

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
          if (!element_is_invisible && input_element->IsTextField()) {
            // Remove non alpha characters from chosen HTML attribute value.
            std::string str;
            if (attribute == NAME_AND_ID)
              str = OnlyKeepLettersInASCII(field.name + field.id);
            else
              str = OnlyKeepLettersInNonASCII(field.label);

            // Search for exact match of word in dictionary.
            size_t length = sizeof(dictionary) / sizeof(char*);
            for (size_t k = 0; k < length; ++k) {
              if (str == dictionary[k]) {
                username_element = *input_element;
                return true;
              }
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
// the field where it appears most times.
// If such a field is found, save it in |username_element| and return true.
// Else, return false, so that the search will continue.
bool SearchForPartialMatch(
    std::vector<blink::WebFormControlElement> control_elements,
    PasswordForm* password_form,
    const char* const* dictionary,
    HTMLAttribute attribute,
    WebInputElement& username_element) {
  size_t last_index = 0;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    WebFormControlElement control_element = control_elements[i];

    for (size_t j = last_index; j < password_form->form_data.fields.size();
         ++j) {
      const FormFieldData& field = password_form->form_data.fields[j];

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
          if (!element_is_invisible && input_element->IsTextField()) {
            // Remove non alpha characters from chosen HTML attribute value.
            std::string str;
            if (attribute == NAME_AND_ID)
              str = OnlyKeepLettersInASCII(field.name + field.id);
            else
              str = OnlyKeepLettersInNonASCII(field.label);

            // Search for partial match of word in dictionary.
            // Get word with as many appearances of a positive word as possible.
            size_t max_occurrences = 0;
            size_t length = sizeof(dictionary) / sizeof(char*);

            for (size_t k = 0; k < length; ++k) {
              if (str.find(dictionary[k]) != std::string::npos &&
                  ContainsNoNegativeWords(str) &&
                  CountSubstring(str, dictionary[k]) > max_occurrences) {
                username_element = *input_element;
                max_occurrences = CountSubstring(str, dictionary[k]);
              }
            }
            if (max_occurrences)
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
  // Do not search for username if a value was already found.
  bool username_found = true;
  if (username_element.IsNull())
    username_found = false;

  // 1. Search in field label.
  // 1.1. Search for "username" as a substring.
  if (!username_found) {
    username_found =
        SearchForPartialMatch(control_elements, password_form,
                              usernameTranslations, LABEL, username_element);
  }

  // 1.2. Search for full words.
  if (!username_found) {
    username_found =
        SearchForFullMatch(control_elements, password_form, userTranslations,
                           LABEL, username_element);
  }
  if (!username_found) {
    username_found =
        SearchForFullMatch(control_elements, password_form, loginTranslations,
                           LABEL, username_element);
  }
  if (!username_found) {
    username_found =
        SearchForFullMatch(control_elements, password_form, idTranslations,
                           LABEL, username_element);
  }

  // 2. Search in field name concatenated with id.
  // 2.1. Search for "username" as a substring.
  if (!username_found) {
    username_found = SearchForPartialMatch(control_elements, password_form,
                                           usernameTranslations, NAME_AND_ID,
                                           username_element);
  }

  // 2.2. Search for positive words as a substring.
  if (!username_found) {
    username_found =
        SearchForPartialMatch(control_elements, password_form, positiveWords,
                              NAME_AND_ID, username_element);
  }

  // 2.3. Search for weak terms (that might indicate to username field) as a
  // substring.
  if (!username_found) {
    username_found =
        SearchForPartialMatch(control_elements, password_form, possibleWords,
                              NAME_AND_ID, username_element);
  }
}

}  // namespace autofill
