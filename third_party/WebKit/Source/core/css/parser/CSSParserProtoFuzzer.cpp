// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSParserProtoFuzzer.h"
#include "core/css/StyleSheetContents.h"
#include "core/css/parser/CSSParser.h"
#include "third_party/WebKit/Source/core/css/parser/CSS.pb.h"

#include <string>
#include <unordered_map>

#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "platform/testing/BlinkFuzzerTestSupport.h"
#include "platform/wtf/text/WTFString.h"

protobuf_mutator::protobuf::LogSilencer log_silencer;

namespace css_parser_proto_fuzzer {

Converter::Converter() {
  s = "";
}

WTF::String Converter::Convert(const StyleSheet& style_sheet_message) {
  Visit(style_sheet_message);
  WTF::String style_sheet_string(s.c_str());
  Reset();
  return style_sheet_string;
}

void Converter::Visit(const Unicode& unicode) {
  s += "\\";
  s += (char)unicode.ascii_value_1();

  if (unicode.has_ascii_value_2())
    s += (char)unicode.ascii_value_2();
  if (unicode.has_ascii_value_3())
    s += (char)unicode.ascii_value_3();
  if (unicode.has_ascii_value_4())
    s += (char)unicode.ascii_value_4();
  if (unicode.has_ascii_value_5())
    s += (char)unicode.ascii_value_5();
  if (unicode.has_ascii_value_6())
    s += (char)unicode.ascii_value_6();

  if (unicode.has_unrepeated_w())
    Visit(unicode.unrepeated_w());
}

void Converter::Visit(const Escape& escape) {
  if (escape.has_ascii_value()) {
    s += "\\";
    s += (char)escape.ascii_value();
  } else if (escape.has_unicode()) {
    Visit(escape.unicode());
  }
}

void Converter::Visit(const Nmstart& nmstart) {
  if (nmstart.has_ascii_value())
    s += (char)nmstart.ascii_value();
  else if (nmstart.has_escape())
    Visit(nmstart.escape());
}

void Converter::Visit(const Nmchar& nmchar) {
  if (nmchar.has_ascii_value())
    s += (char)nmchar.ascii_value();
  else if (nmchar.has_escape())
    Visit(nmchar.escape());
}

void Converter::Visit(const String& string) {
  bool use_single_quotes = string.use_single_quotes();
  if (use_single_quotes)
    s += "'";
  else
    s += "\"";

  for (auto& string_char_quote : string.string_char_quotes())
    Visit(string_char_quote, use_single_quotes);

  if (use_single_quotes)
    s += "'";
  else
    s += "\"";
}

void Converter::Visit(const StringCharOrQuote& string_char_quote,
                      bool using_single_quote) {
  if (string_char_quote.has_string_char()) {
    Visit(string_char_quote.string_char());
  } else if (string_char_quote.quote_char()) {
    if (using_single_quote)
      s += "\"";
    else
      s += "'";
  }
}

void Converter::Visit(const StringChar& string_char) {
  if (string_char.has_url_char())
    Visit(string_char.url_char());
  else if (string_char.has_space())
    s += " ";
  else if (string_char.has_nl())
    Visit(string_char.nl());
}

void Converter::Visit(const Ident& ident) {
  if (ident.starting_minus())
    s += "-";
  Visit(ident.nmstart());
  ;
  for (auto& nmchar : ident.nmchars())
    Visit(nmchar);
}

void Converter::Visit(const Name& name) {
  Visit(name.first_nmchar());
  for (auto& nmchar : name.later_nmchars())
    Visit(nmchar);
}

void Converter::Visit(const Num& num) {
  if (num.has_float_value())
    s += std::to_string(num.float_value());
  else if (num.has_signed_int_value())
    s += std::to_string(num.signed_int_value());
}

void Converter::Visit(const UrlChar& url_char) {
  s += (char)url_char.ascii_value();
}

// TODO(metzman): implement W
void Converter::Visit(const UnrepeatedW& unrepeated_w) {
  s += (char)unrepeated_w.ascii_value();
}

void Converter::Visit(const Nl& nl) {
  s += "\\";
  if (nl.newline_kind() == Nl::CR_LF)
    s += "\r\n";
  else  // Otherwise newline_kind is the ascii value of the char we want.
    s += (char)nl.newline_kind();
}

void Converter::Visit(const Length& length) {
  Visit(length.num());
  if (length.unit() == Length::PX)
    s += "px";
  else if (length.unit() == Length::CM)
    s += "cm";
  else if (length.unit() == Length::MM)
    s += "mm";
  else if (length.unit() == Length::IN)
    s += "in";
  else if (length.unit() == Length::PT)
    s += "pt";
  else if (length.unit() == Length::PC)
    s += "pc";
  else
    NOTREACHED();
}

void Converter::Visit(const Angle& angle) {
  Visit(angle.num());
  if (angle.unit() == Angle::DEG)
    s += "deg";
  else if (angle.unit() == Angle::RAD)
    s += "rad";
  else if (angle.unit() == Angle::GRAD)
    s += "grad";
  else
    NOTREACHED();
}

void Converter::Visit(const Time& time) {
  Visit(time.num());
  if (time.unit() == Time::MS)
    s += "ms";
  else if (time.unit() == Time::S)
    s += "s";
  else
    NOTREACHED();
}

void Converter::Visit(const Freq& freq) {
  Visit(freq.num());
  // Hack around really dumb build bug
  if (freq.unit() == Freq::_HZ)
    s += "Hz";
  else if (freq.unit() == Freq::KHZ)
    s += "kHz";
  else
    NOTREACHED();
}

void Converter::Visit(const Uri& uri) {
  s += "url(";
  Visit(uri.value());
  s += ")";
}

void Converter::Visit(const FunctionToken& function_token) {
  Visit(function_token.ident());
  s += "(";
}

void Converter::Visit(const StyleSheet& style_sheet) {
  if (style_sheet.has_charset_declaration())
    Visit(style_sheet.charset_declaration());
  for (auto& import : style_sheet.imports())
    Visit(import);
  for (auto& _namespace : style_sheet.namespaces())
    Visit(_namespace);
  for (auto& ruleset_media_page_font_face :
       style_sheet.ruleset_media_page_font_faces())
    Visit(ruleset_media_page_font_face);
}

void Converter::Visit(const CharsetDeclaration& charset_declaration) {
  s += "@charset";  // CHARSET_SYM
  Visit(charset_declaration.string());
  s += "; ";
}

void Converter::Visit(
    const RulesetOrMediaOrPageOrFontFace& ruleset_media_page_font_face) {
  if (ruleset_media_page_font_face.has_ruleset())
    Visit(ruleset_media_page_font_face.ruleset());
  if (ruleset_media_page_font_face.has_media())
    Visit(ruleset_media_page_font_face.media());
  else if (ruleset_media_page_font_face.has_page())
    Visit(ruleset_media_page_font_face.page());
  else if (ruleset_media_page_font_face.has_font_face())
    Visit(ruleset_media_page_font_face.font_face());
}

void Converter::Visit(const Import& import) {
  s += "@import ";
  if (import.has_uri())
    Visit(import.uri());
  else if (import.has_string())
    Visit(import.string());
  if (import.has_medium_list())
    Visit(import.medium_list());
  s += "; ";
}

void Converter::Visit(const MediumList& medium_list) {
  Visit(medium_list.first_medium());
  for (auto& medium : medium_list.later_mediums()) {
    s += ", ";
    Visit(medium);
  }
  s += " ";
}

void Converter::Visit(const Namespace& _namespace) {
  s += "@namespace ";
  if (_namespace.has_namespace_prefix())
    Visit(_namespace.namespace_prefix());
  if (_namespace.has_string())
    Visit(_namespace.string());
  if (_namespace.has_uri())
    Visit(_namespace.uri());

  s += "; ";
}

void Converter::Visit(const NamespacePrefix& namespace_prefix) {
  Visit(namespace_prefix.ident());
}

void Converter::Visit(const Media& media) {
  // MEDIA_SYM S*
  s += "@media ";  // "@media" {return MEDIA_SYM;}

  Visit(media.medium_list());
  s += " { ";
  for (auto& ruleset : media.rulesets())
    Visit(ruleset);
  s += " } ";
}

void Converter::Visit(const Medium& medium) {
  Visit(medium.ident());
  s += " ";
}

void Converter::Visit(const Page& page) {
  // PAGE_SYM
  s += "@page ";  // PAGE_SYM
  if (page.has_ident())
    Visit(page.ident());
  if (page.has_pseudo_page())
    Visit(page.pseudo_page());
  s += " { ";
  Visit(page.declaration_list());
  s += " } ";
}

void Converter::Visit(const DeclarationList& declaration_list) {
  Visit(declaration_list.first_declaration());
  for (auto& declaration : declaration_list.later_declarations()) {
    s += ";";
    Visit(declaration);
  }
}

void Converter::Visit(const PseudoPage& pseudo_page) {
  s += ":";
  Visit(pseudo_page.ident());
}

void Converter::Visit(const FontFace& font_face) {
  s += "@font-face";
  s += "{";
  Visit(font_face.declaration_list());
  s += "}";
}

void Converter::Visit(const Operator& _operator) {
  if (_operator.has_ascii_value())
    s += (char)_operator.ascii_value();
}

void Converter::Visit(const Combinator& combinator) {
  if (combinator.has_ascii_value())
    s += (char)combinator.ascii_value();
}

void Converter::Visit(const UnaryOperator& unary_operator) {
  s += (char)unary_operator.ascii_value();
}

void Converter::Visit(const Property& property) {
  Visit(property.ident());
}

void Converter::Visit(const Ruleset& ruleset) {
  Visit(ruleset.selector_list());
  s += " {";
  Visit(ruleset.declaration_list());
  s += "} ";
}

void Converter::Visit(const SelectorList& selector_list) {
  Visit(selector_list.first_selector());
  for (auto& selector : selector_list.later_selectors()) {
    s += ",";
    Visit(selector);
  }
}

void Converter::Visit(const Selector& selector) {
  Visit(selector.simple_selector());
  for (auto& combinator_simple_selector :
       selector.combinator_simple_selectors())
    Visit(combinator_simple_selector);
}

void Converter::Visit(
    const CombinatorAndSimpleSelector& combinator_simple_selector) {
  Visit(combinator_simple_selector.combinator());
  Visit(combinator_simple_selector.simple_selector());
}

void Converter::Visit(const SimpleSelector& simple_selector) {
  if (simple_selector.has_element_name())
    Visit(simple_selector.element_name());
  for (auto& hash_class_attrib_pseudo :
       simple_selector.hash_class_attrib_pseudos())
    Visit(hash_class_attrib_pseudo);
}

void Converter::Visit(
    const HashOrClassOrAttribOrPseudo& hash_class_attrib_pseudo) {
  if (hash_class_attrib_pseudo.has_hash()) {
    s += "#";
    Visit(hash_class_attrib_pseudo.hash());
  } else if (hash_class_attrib_pseudo.has__class()) {
    Visit(hash_class_attrib_pseudo._class());
  } else if (hash_class_attrib_pseudo.has_attrib()) {
    Visit(hash_class_attrib_pseudo.attrib());
  } else if (hash_class_attrib_pseudo.has_pseudo()) {
    Visit(hash_class_attrib_pseudo.pseudo());
  }
}

void Converter::Visit(const _Class& _class) {
  s += ".";
  Visit(_class.ident());
}

void Converter::Visit(const Attrib& attrib) {
  s += "[";
  Visit(attrib.ident());
  if (attrib.has_attrib_part_two())
    Visit(attrib.attrib_part_two());
  s += "]";
}

void Converter::Visit(const AttribPartTwo& attrib_part_two) {
  if (attrib_part_two.equal_includes_dashmatch() == AttribPartTwo::EQUAL)
    s += "=";
  else if (attrib_part_two.equal_includes_dashmatch() ==
           AttribPartTwo::INCLUDES)
    s += "~=";
  else if (attrib_part_two.equal_includes_dashmatch() ==
           AttribPartTwo::DASHMATCH)
    s += "|=";

  if (attrib_part_two.has_ident())
    Visit(attrib_part_two.ident());
  if (attrib_part_two.has_string())
    Visit(attrib_part_two.string());
}

void Converter::Visit(const Pseudo& pseudo) {
  s += ":";

  if (pseudo.has_ident_1())
    Visit(pseudo.ident_1());
  else if (pseudo.has_function_token())
    Visit(pseudo.function_token());

  Visit(pseudo.ident_2());
  s += ")";
}

void Converter::Visit(const Declaration& declaration) {
  if (declaration.has_nonempty_declaration())
    Visit(declaration.nonempty_declaration());
  // else empty
}

void Converter::Visit(const NonEmptyDeclaration& nonempty_declaration) {
  Visit(nonempty_declaration.property());
  s += ":";
  s += " ";  // S*
  Visit(nonempty_declaration.expr());
  if (nonempty_declaration.has_prio())
    s += "!important";
}

void Converter::Visit(const ElementName& element_name) {
  if (element_name.has_star())
    s += "*";
  else if (element_name.has_ident())
    Visit(element_name.ident());
}

void Converter::Visit(const Expr& expr) {
  Visit(expr.term());
  for (auto& operator_term : expr.operator_terms())
    Visit(operator_term);
}

void Converter::Visit(const OperatorTerm& operator_term) {
  Visit(operator_term._operator());
  Visit(operator_term.term());
}

void Converter::Visit(const Term& term) {
  if (term.has_unary_operator())
    Visit(term.unary_operator());

  if (term.has_term_part())
    Visit(term.term_part());
  else if (term.has_string())
    Visit(term.string());

  if (term.has_ident())
    Visit(term.ident());
  if (term.has_uri())
    Visit(term.uri());
  if (term.has_hexcolor())
    Visit(term.hexcolor());
}

void Converter::Visit(const TermPart& term_part) {
  if (term_part.has_number())
    Visit(term_part.number());
  // S* | PERCENTAGE
  if (term_part.has_percentage()) {
    Visit(term_part.percentage());
    s += "%";
  }
  // S* | LENGTH
  if (term_part.has_length())
    Visit(term_part.length());
  // S* | EMS
  if (term_part.has_ems()) {
    Visit(term_part.ems());
    s += "em";
  }
  // S* | EXS
  if (term_part.has_exs()) {
    Visit(term_part.exs());
    s += "ex";
  }
  // S* | Angle
  if (term_part.has_angle())
    Visit(term_part.angle());
  // S* | TIME
  if (term_part.has_time())
    Visit(term_part.time());
  // S* | FREQ
  if (term_part.has_freq())
    Visit(term_part.freq());
  // S* | function
  if (term_part.has_function()) {
    Visit(term_part.function());
  }
}

void Converter::Visit(const Function& function) {
  Visit(function.function_token());
  Visit(function.expr());
  s += ")";
}

void Converter::Visit(const Hexcolor& hexcolor) {
  s += "#";
  Visit(hexcolor.first_three());
  if (hexcolor.has_last_three())
    Visit(hexcolor.last_three());
}

void Converter::Visit(const HexcolorThree& hexcolor_three) {
  s += (char)hexcolor_three.ascii_value_1();
  s += (char)hexcolor_three.ascii_value_2();
  s += (char)hexcolor_three.ascii_value_3();
}

void Converter::Reset() {
  s = "";
}

DEFINE_BINARY_PROTO_FUZZER(const Input& input) {
  static Converter* converter;
  static bool initialized = false;
  static std::unordered_map<Input::CSSParserMode, blink::CSSParserMode>
      parser_mode_map = {
    {Input::kHTMLStandardMode, blink::kHTMLStandardMode},
    {Input::kHTMLQuirksMode, blink::kHTMLQuirksMode},
    {Input::kSVGAttributeMode, blink::kSVGAttributeMode},
    {Input::kCSSViewportRuleMode, blink::kCSSViewportRuleMode},
    {Input::kCSSFontFaceRuleMode, blink::kCSSFontFaceRuleMode},
    {Input::kUASheetMode, blink::kUASheetMode}
  };

  if (!initialized) {
    static blink::BlinkFuzzerTestSupport test_support =
        blink::BlinkFuzzerTestSupport();
    initialized = true;
    converter = new Converter();
  }
  blink::CSSParserMode mode = parser_mode_map[input.css_parser_mode()];

  blink::CSSParserContext::SelectorProfile selector_profile;
  if (input.is_dynamic_profile())
    selector_profile = blink::CSSParserContext::kDynamicProfile;
  else
    selector_profile = blink::CSSParserContext::kStaticProfile;
  blink::CSSParserContext* context =
      blink::CSSParserContext::Create(mode, selector_profile);

  blink::StyleSheetContents* style_sheet =
      blink::StyleSheetContents::Create(context);
  WTF::String style_sheet_string = converter->Convert(input.style_sheet());
  blink::CSSParser::ParseSheet(context, style_sheet, style_sheet_string,
                               input.defer_property_parsing());

}
};  // namespace css_parser_proto_fuzzer
