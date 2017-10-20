// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSParserProtoConverter.h"

#include <string>

// TODO(metzman): Figure out how to remove this include and use DCHECK.
#include "platform/wtf/text/WTFString.h"
#include "third_party/WebKit/Source/core/css/parser/CSS.pb.h"

namespace css_parser_proto_fuzzer {

static std::string kPropertyLookupTable[] = {
    "",
    "all",
    "-webkit-animation-iteration-count",
    "font-feature-settings",
    "-webkit-text-emphasis-position",
    "-webkit-text-emphasis-style",
    "grid-template-rows",
    "text-underline-position",
    "-webkit-flex-grow",
    "scroll-snap-margin-right",
    "-webkit-column-rule",
    "-webkit-order",
    "grid-row-gap",
    "backdrop-filter",
    "font-variant-east-asian",
    "buffered-rendering",
    "-webkit-appearance",
    "outline-width",
    "alignment-baseline",
    "-webkit-flex-flow",
    "column-rule",
    "grid-column-gap",
    "-webkit-border-after",
    "-webkit-column-break-inside",
    "-webkit-shape-outside",
    "-webkit-print-color-adjust",
    "list-style-type",
    "page-break-before",
    "flood-color",
    "text-anchor",
    "-webkit-padding-start",
    "-webkit-user-select",
    "-webkit-column-rule-color",
    "padding-left",
    "-webkit-backface-visibility",
    "-webkit-margin-before",
    "break-inside",
    "column-count",
    "-webkit-logical-height",
    "perspective",
    "max-block-size",
    "-webkit-animation-play-state",
    "border-image-repeat",
    "-webkit-font-size-delta",
    "scroll-padding-bottom",
    "border-right-style",
    "border-left-style",
    "scroll-snap-margin-block",
    "flex-flow",
    "outline-color",
    "flex-grow",
    "max-width",
    "grid-column",
    "image-orientation",
    "animation-duration",
    "-webkit-columns",
    "-webkit-box-flex-group",
    "-webkit-animation-delay",
    "-epub-text-emphasis",
    "flex-shrink",
    "text-rendering",
    "align-items",
    "border-collapse",
    "offset",
    "text-combine-upright",
    "-webkit-mask-position-x",
    "-webkit-mask-position-y",
    "outline-style",
    "-webkit-margin-bottom-collapse",
    "color-interpolation-filters",
    "font-variant",
    "-webkit-animation-fill-mode",
    "border-right",
    "visibility",
    "transform-box",
    "font-variant-caps",
    "-epub-text-emphasis-color",
    "-webkit-border-before-style",
    "resize",
    "-webkit-rtl-ordering",
    "-webkit-box-ordinal-group",
    "paint-order",
    "stroke-linecap",
    "animation-direction",
    "-webkit-font-feature-settings",
    "border-top-left-radius",
    "-webkit-column-width",
    "-webkit-box-align",
    "-webkit-padding-after",
    "column-width",
    "list-style",
    "-webkit-mask-repeat-y",
    "-webkit-margin-before-collapse",
    "stroke",
    "text-decoration-line",
    "-webkit-background-size",
    "-webkit-mask-repeat-x",
    "padding-bottom",
    "font-style",
    "-webkit-transition-delay",
    "background-repeat",
    "flex-basis",
    "border-image-slice",
    "-webkit-transform-origin",
    "scroll-boundary-behavior-x",
    "scroll-boundary-behavior-y",
    "vector-effect",
    "-webkit-animation-timing-function",
    "-webkit-border-after-style",
    "-webkit-perspective-origin-x",
    "-webkit-perspective-origin-y",
    "inline-size",
    "outline",
    "font-display",
    "-webkit-border-before",
    "border-image-source",
    "transition-duration",
    "scroll-padding-top",
    "order",
    "-webkit-box-orient",
    "counter-reset",
    "color-rendering",
    "flex-direction",
    "-webkit-text-stroke-width",
    "font-variant-numeric",
    "scroll-snap-margin-block-end",
    "min-height",
    "scroll-padding-inline-start",
    "-webkit-mask-box-image",
    "left",
    "-webkit-mask",
    "-webkit-border-after-width",
    "stroke-width",
    "-webkit-box-decoration-break",
    "-webkit-mask-position",
    "background-origin",
    "-webkit-border-start-color",
    "font-stretch",
    "-webkit-background-clip",
    "scroll-snap-margin-top",
    "-webkit-border-horizontal-spacing",
    "border-radius",
    "flex",
    "text-indent",
    "hyphens",
    "column-rule-width",
    "-webkit-margin-after",
    "-epub-caption-side",
    "break-after",
    "text-transform",
    "touch-action",
    "font-size",
    "-webkit-animation-name",
    "scroll-padding-inline",
    "offset-path",
    "scroll-snap-margin",
    "padding-top",
    "scroll-snap-align",
    "-webkit-text-combine",
    "-webkit-flex-shrink",
    "rx",
    "ry",
    "content",
    "padding-right",
    "-webkit-transform",
    "marker-mid",
    "-webkit-min-logical-width",
    "clip-rule",
    "font-family",
    "scroll-snap-type",
    "text-decoration-skip",
    "transition",
    "filter",
    "border-right-width",
    "-webkit-flex-direction",
    "-webkit-mask-composite",
    "mix-blend-mode",
    "color-interpolation",
    "border-top-style",
    "fill-opacity",
    "marker-start",
    "border-bottom-width",
    "-webkit-text-emphasis",
    "grid-area",
    "size",
    "background-clip",
    "-webkit-text-fill-color",
    "top",
    "-webkit-box-reflect",
    "border-width",
    "offset-anchor",
    "max-inline-size",
    "-webkit-column-rule-style",
    "-webkit-column-count",
    "animation-play-state",
    "padding",
    "dominant-baseline",
    "background-attachment",
    "-webkit-box-sizing",
    "-webkit-box-flex",
    "text-orientation",
    "background-position",
    "-webkit-border-start-width",
    "-epub-text-emphasis-style",
    "isolation",
    "-epub-text-orientation",
    "-webkit-border-bottom-right-radius",
    "r",
    "border-left-width",
    "grid-column-end",
    "background-blend-mode",
    "vertical-align",
    "clip",
    "grid-auto-rows",
    "offset-rotate",
    "margin-left",
    "animation-name",
    "text-decoration",
    "border",
    "-webkit-transition-timing-function",
    "margin-bottom",
    "unicode-range",
    "animation",
    "-webkit-shape-margin",
    "font-weight",
    "shape-margin",
    "mask-type",
    "scroll-padding",
    "min-inline-size",
    "object-position",
    "page-break-after",
    "-webkit-mask-clip",
    "white-space",
    "-webkit-border-after-color",
    "-webkit-max-logical-width",
    "-webkit-border-before-color",
    "font-kerning",
    "-epub-word-break",
    "clear",
    "animation-timing-function",
    "-webkit-border-radius",
    "scroll-padding-right",
    "-webkit-text-decorations-in-effect",
    "-webkit-animation-direction",
    "justify-self",
    "transition-timing-function",
    "scroll-snap-stop",
    "counter-increment",
    "-webkit-transform-style",
    "grid-auto-columns",
    "-webkit-align-content",
    "font",
    "flex-wrap",
    "grid-row-start",
    "list-style-image",
    "-webkit-tap-highlight-color",
    "-webkit-text-emphasis-color",
    "border-left",
    "-webkit-border-end-color",
    "columns",
    "box-shadow",
    "-webkit-flex-wrap",
    "align-self",
    "border-bottom",
    "border-spacing",
    "-webkit-column-span",
    "grid-row-end",
    "-webkit-border-end",
    "perspective-origin",
    "page-break-inside",
    "orphans",
    "-webkit-border-start-style",
    "scroll-behavior",
    "column-span",
    "-webkit-hyphenate-character",
    "column-fill",
    "tab-size",
    "contain",
    "x",
    "grid-row",
    "border-bottom-right-radius",
    "line-height",
    "stroke-linejoin",
    "text-align-last",
    "offset-position",
    "word-spacing",
    "transform-style",
    "-webkit-app-region",
    "-webkit-border-end-style",
    "-webkit-transform-origin-z",
    "-webkit-transform-origin-x",
    "-webkit-transform-origin-y",
    "background-repeat-x",
    "background-repeat-y",
    "border-bottom-color",
    "-webkit-ruby-position",
    "-webkit-logical-width",
    "text-justify",
    "scroll-snap-margin-inline-start",
    "caption-side",
    "mask-source-type",
    "-webkit-mask-box-image-slice",
    "-webkit-border-image",
    "text-size-adjust",
    "-webkit-text-security",
    "-epub-writing-mode",
    "grid-template",
    "-webkit-mask-box-image-repeat",
    "-webkit-mask-repeat",
    "-webkit-justify-content",
    "baseline-shift",
    "border-image",
    "text-decoration-color",
    "color",
    "shape-image-threshold",
    "shape-rendering",
    "cy",
    "cx",
    "-webkit-user-modify",
    "offset-distance",
    "-webkit-border-bottom-left-radius",
    "speak",
    "border-bottom-left-radius",
    "-webkit-column-break-after",
    "-webkit-font-smoothing",
    "-webkit-max-logical-height",
    "-webkit-line-break",
    "fill-rule",
    "-webkit-margin-start",
    "min-width",
    "-epub-text-combine",
    "break-before",
    "caret-color",
    "empty-cells",
    "direction",
    "clip-path",
    "justify-content",
    "scroll-padding-block-end",
    "z-index",
    "background-position-y",
    "text-decoration-style",
    "grid-template-areas",
    "-webkit-min-logical-height",
    "font-size-adjust",
    "scroll-padding-block",
    "overflow-anchor",
    "cursor",
    "scroll-snap-margin-block-start",
    "-webkit-mask-box-image-source",
    "margin",
    "-webkit-animation",
    "letter-spacing",
    "orientation",
    "will-change",
    "-webkit-highlight",
    "transform-origin",
    "font-variant-ligatures",
    "-webkit-animation-duration",
    "-webkit-mask-origin",
    "-webkit-clip-path",
    "word-break",
    "table-layout",
    "text-overflow",
    "-webkit-locale",
    "-webkit-flex",
    "grid-auto-flow",
    "border-top-right-radius",
    "border-image-outset",
    "place-items",
    "border-left-color",
    "font-variation-settings",
    "border-right-color",
    "min-zoom",
    "scroll-snap-margin-inline",
    "-webkit-border-before-width",
    "backface-visibility",
    "background-image",
    "-webkit-transition-property",
    "writing-mode",
    "stroke-opacity",
    "-webkit-margin-collapse",
    "box-sizing",
    "margin-top",
    "column-rule-color",
    "y",
    "position",
    "scroll-snap-margin-bottom",
    "list-style-position",
    "-webkit-box-pack",
    "scroll-padding-inline-end",
    "quotes",
    "border-top",
    "scroll-padding-left",
    "-webkit-transition",
    "-webkit-column-break-before",
    "lighting-color",
    "background-size",
    "-webkit-padding-before",
    "-webkit-border-top-left-radius",
    "flood-opacity",
    "line-height-step",
    "-webkit-mask-size",
    "text-align",
    "-webkit-filter",
    "word-wrap",
    "max-zoom",
    "grid",
    "background",
    "height",
    "grid-column-start",
    "animation-fill-mode",
    "rotate",
    "marker-end",
    "d",
    "justify-items",
    "zoom",
    "scroll-padding-block-start",
    "-webkit-margin-top-collapse",
    "page",
    "right",
    "user-select",
    "margin-right",
    "marker",
    "line-break",
    "-webkit-margin-end",
    "-webkit-transition-duration",
    "-webkit-writing-mode",
    "border-top-width",
    "bottom",
    "place-content",
    "-webkit-shape-image-threshold",
    "-webkit-user-drag",
    "-webkit-border-vertical-spacing",
    "-webkit-column-gap",
    "-webkit-opacity",
    "background-color",
    "column-gap",
    "shape-outside",
    "-webkit-padding-end",
    "-webkit-border-start",
    "animation-delay",
    "unicode-bidi",
    "text-shadow",
    "-webkit-box-direction",
    "image-rendering",
    "src",
    "grid-gap",
    "pointer-events",
    "border-image-width",
    "min-block-size",
    "transition-property",
    "-webkit-mask-image",
    "float",
    "max-height",
    "outline-offset",
    "-webkit-box-shadow",
    "overflow-wrap",
    "block-size",
    "transform",
    "place-self",
    "width",
    "stroke-miterlimit",
    "stop-opacity",
    "border-top-color",
    "translate",
    "object-fit",
    "-webkit-mask-box-image-width",
    "-webkit-background-origin",
    "-webkit-align-items",
    "transition-delay",
    "scroll-snap-margin-left",
    "border-style",
    "animation-iteration-count",
    "-webkit-margin-after-collapse",
    "overflow",
    "user-zoom",
    "-webkit-border-top-right-radius",
    "grid-template-columns",
    "-webkit-align-self",
    "-webkit-perspective-origin",
    "column-rule-style",
    "display",
    "-webkit-column-rule-width",
    "-webkit-box-lines",
    "border-color",
    "-webkit-flex-basis",
    "stroke-dashoffset",
    "-webkit-text-size-adjust",
    "scroll-boundary-behavior",
    "-webkit-text-stroke",
    "widows",
    "fill",
    "overflow-y",
    "overflow-x",
    "opacity",
    "-webkit-perspective",
    "-webkit-text-stroke-color",
    "scroll-snap-margin-inline-end",
    "scale",
    "-webkit-text-orientation",
    "-webkit-mask-box-image-outset",
    "align-content",
    "-webkit-border-end-width",
    "border-bottom-style",
    "mask",
    "background-position-x",
    "-epub-text-transform",
    "stop-color",
    "stroke-dasharray",
    "-webkit-line-clamp",
};

Converter::Converter() {}

#include <iostream>
std::string Converter::Convert(const StyleSheet& style_sheet_message) {
  Reset();
  Visit(style_sheet_message);
  return string_;
}

void Converter::Visit(const Unicode& unicode) {
  string_ += "\\";
  string_ += static_cast<char>(unicode.ascii_value_1());

  if (unicode.has_ascii_value_2())
    string_ += static_cast<char>(unicode.ascii_value_2());
  if (unicode.has_ascii_value_3())
    string_ += static_cast<char>(unicode.ascii_value_3());
  if (unicode.has_ascii_value_4())
    string_ += static_cast<char>(unicode.ascii_value_4());
  if (unicode.has_ascii_value_5())
    string_ += static_cast<char>(unicode.ascii_value_5());
  if (unicode.has_ascii_value_6())
    string_ += static_cast<char>(unicode.ascii_value_6());

  if (unicode.has_unrepeated_w())
    Visit(unicode.unrepeated_w());
}

void Converter::Visit(const Escape& escape) {
  if (escape.has_ascii_value()) {
    string_ += "\\";
    string_ += static_cast<char>(escape.ascii_value());
  } else if (escape.has_unicode()) {
    Visit(escape.unicode());
  }
}

void Converter::Visit(const Nmstart& nmstart) {
  if (nmstart.has_ascii_value())
    string_ += static_cast<char>(nmstart.ascii_value());
  else if (nmstart.has_escape())
    Visit(nmstart.escape());
}

void Converter::Visit(const Nmchar& nmchar) {
  if (nmchar.has_ascii_value())
    string_ += static_cast<char>(nmchar.ascii_value());
  else if (nmchar.has_escape())
    Visit(nmchar.escape());
}

void Converter::Visit(const String& string) {
  bool use_single_quotes = string.use_single_quotes();
  if (use_single_quotes)
    string_ += "'";
  else
    string_ += "\"";

  for (auto& string_char_quote : string.string_char_quotes())
    Visit(string_char_quote, use_single_quotes);

  if (use_single_quotes)
    string_ += "'";
  else
    string_ += "\"";
}

void Converter::Visit(const StringCharOrQuote& string_char_quote,
                      bool using_single_quote) {
  if (string_char_quote.has_string_char()) {
    Visit(string_char_quote.string_char());
  } else if (string_char_quote.quote_char()) {
    if (using_single_quote)
      string_ += "\"";
    else
      string_ += "'";
  }
}

void Converter::Visit(const StringChar& string_char) {
  if (string_char.has_url_char())
    Visit(string_char.url_char());
  else if (string_char.has_space())
    string_ += " ";
  else if (string_char.has_nl())
    Visit(string_char.nl());
}

void Converter::Visit(const Ident& ident) {
  if (ident.starting_minus())
    string_ += "-";
  Visit(ident.nmstart());
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
    string_ += std::to_string(num.float_value());
  else if (num.has_signed_int_value())
    string_ += std::to_string(num.signed_int_value());
}

void Converter::Visit(const UrlChar& url_char) {
  string_ += static_cast<char>(url_char.ascii_value());
}

// TODO(metzman): implement W
void Converter::Visit(const UnrepeatedW& unrepeated_w) {
  string_ += static_cast<char>(unrepeated_w.ascii_value());
}

void Converter::Visit(const Nl& nl) {
  string_ += "\\";
  if (nl.newline_kind() == Nl::CR_LF)
    string_ += "\r\n";
  else  // Otherwise newline_kind is the ascii value of the char we want.
    string_ += static_cast<char>(nl.newline_kind());
}

void Converter::Visit(const Length& length) {
  Visit(length.num());
  if (length.unit() == Length::PX)
    string_ += "px";
  else if (length.unit() == Length::CM)
    string_ += "cm";
  else if (length.unit() == Length::MM)
    string_ += "mm";
  else if (length.unit() == Length::IN)
    string_ += "in";
  else if (length.unit() == Length::PT)
    string_ += "pt";
  else if (length.unit() == Length::PC)
    string_ += "pc";
  else
    NOTREACHED();
}

void Converter::Visit(const Angle& angle) {
  Visit(angle.num());
  if (angle.unit() == Angle::DEG)
    string_ += "deg";
  else if (angle.unit() == Angle::RAD)
    string_ += "rad";
  else if (angle.unit() == Angle::GRAD)
    string_ += "grad";
  else
    NOTREACHED();
}

void Converter::Visit(const Time& time) {
  Visit(time.num());
  if (time.unit() == Time::MS)
    string_ += "ms";
  else if (time.unit() == Time::S)
    string_ += "s";
  else
    NOTREACHED();
}

void Converter::Visit(const Freq& freq) {
  Visit(freq.num());
  // Hack around really dumb build bug
  if (freq.unit() == Freq::_HZ)
    string_ += "Hz";
  else if (freq.unit() == Freq::KHZ)
    string_ += "kHz";
  else
    NOTREACHED();
}

void Converter::Visit(const Uri& uri) {
  string_ += "url(";
  Visit(uri.value());
  string_ += ")";
}

void Converter::Visit(const FunctionToken& function_token) {
  Visit(function_token.ident());
  string_ += "(";
}

void Converter::Visit(const StyleSheet& style_sheet) {
  // if (style_sheet.has_charset_declaration())
  //   Visit(style_sheet.charset_declaration());
  // for (auto& import : style_sheet.imports())
  //   Visit(import);
  for (auto& _namespace : style_sheet.namespaces())
    Visit(_namespace);
  for (auto& ruleset_media_page_font_face :
       style_sheet.ruleset_media_page_font_faces())
    Visit(ruleset_media_page_font_face);
}

void Converter::Visit(const CharsetDeclaration& charset_declaration) {
  string_ += "@charset";  // CHARSET_SYM
  Visit(charset_declaration.string());
  string_ += "; ";
}

void Converter::Visit(
    const RulesetOrMediaOrPageOrFontFace& ruleset_media_page_font_face) {
  if (ruleset_media_page_font_face.has_ruleset())
    Visit(ruleset_media_page_font_face.ruleset());
  if (ruleset_media_page_font_face.has_media())
    Visit(ruleset_media_page_font_face.media());
  // else if (ruleset_media_page_font_face.has_page())
  //   Visit(ruleset_media_page_font_face.page());
  else if (ruleset_media_page_font_face.has_font_face())
    Visit(ruleset_media_page_font_face.font_face());
}

void Converter::Visit(const Import& import) {
  string_ += "@import ";
  if (import.has_uri())
    Visit(import.uri());
  else if (import.has_string())
    Visit(import.string());
  if (import.has_medium_list())
    Visit(import.medium_list());
  string_ += "; ";
}

void Converter::Visit(const MediumList& medium_list) {
  Visit(medium_list.first_medium());
  for (auto& medium : medium_list.later_mediums()) {
    string_ += ", ";
    Visit(medium);
  }
  string_ += " ";
}

void Converter::Visit(const Namespace& _namespace) {
  string_ += "@namespace ";
  if (_namespace.has_namespace_prefix())
    Visit(_namespace.namespace_prefix());
  if (_namespace.has_string())
    Visit(_namespace.string());
  if (_namespace.has_uri())
    Visit(_namespace.uri());

  string_ += "; ";
}

void Converter::Visit(const NamespacePrefix& namespace_prefix) {
  Visit(namespace_prefix.ident());
}

// TODO(metzman): This is probably very broken. See
// https://www.w3.org/TR/css3-mediaqueries/#syntax for how to properly generate
// media stuff.
void Converter::Visit(const Media& media) {
  // MEDIA_SYM S*
  string_ += "@media ";  // "@media" {return MEDIA_SYM;}

  Visit(media.medium_list());
  string_ += " { ";
  for (auto& ruleset : media.rulesets())
    Visit(ruleset);
  string_ += " } ";
}

void Converter::Visit(const Medium& medium) {
  Visit(medium.ident());
  string_ += " ";
}

void Converter::Visit(const Page& page) {
  // PAGE_SYM
  string_ += "@page ";  // PAGE_SYM
  if (page.has_ident())
    Visit(page.ident());
  if (page.has_pseudo_page())
    Visit(page.pseudo_page());
  string_ += " { ";
  Visit(page.declaration_list());
  string_ += " } ";
}

void Converter::Visit(const DeclarationList& declaration_list) {
  Visit(declaration_list.first_declaration());
  for (auto& declaration : declaration_list.later_declarations()) {
    Visit(declaration);
    string_ += "; ";
  }
}

void Converter::Visit(const PseudoPage& pseudo_page) {
  string_ += ":";
  Visit(pseudo_page.ident());
}

void Converter::Visit(const FontFace& font_face) {
  string_ += "@font-face";
  string_ += "{";
  Visit(font_face.declaration_list());
  string_ += "}";
}

void Converter::Visit(const Operator& _operator) {
  if (_operator.has_ascii_value())
    string_ += static_cast<char>(_operator.ascii_value());
}

void Converter::Visit(const Combinator& combinator) {
  if (combinator.has_ascii_value())
    string_ += static_cast<char>(combinator.ascii_value());
}

void Converter::Visit(const UnaryOperator& unary_operator) {
  string_ += static_cast<char>(unary_operator.ascii_value());
}

void Converter::Visit(const Property& property) {
  // Visit(property.ident());
  // TODO: change this to a map.

  int property_id = static_cast<int>(property.name());
  DCHECK(property_id > 0 &&
         static_cast<size_t>(property_id) < sizeof(kPropertyLookupTable));

  string_ += kPropertyLookupTable[property_id];
}

void Converter::Visit(const Ruleset& ruleset) {
  // Visit(ruleset.selector_list());
  string_ += " x{";
  Visit(ruleset.declaration_list());
  string_ += "} ";
}

void Converter::Visit(const SelectorList& selector_list) {
  Visit(selector_list.first_selector());
  // for (auto& selector : selector_list.later_selectors()) {
  //   string_ += ",";
  //   Visit(selector);
  // }
}

void Converter::Visit(const Selector& selector) {
  Visit(selector.simple_selector());
  // for (auto& combinator_simple_selector :
  //      selector.combinator_simple_selectors())
  //   Visit(combinator_simple_selector);
  // string_ += selector.selector_string();
}

void Converter::Visit(
    const CombinatorAndSimpleSelector& combinator_simple_selector) {
  Visit(combinator_simple_selector.combinator());
  Visit(combinator_simple_selector.simple_selector());
}

void Converter::Visit(const SimpleSelector& simple_selector) {
  if (simple_selector.has_element_name())
    Visit(simple_selector.element_name());
  // for (auto& hash_class_attrib_pseudo :
  //      simple_selector.hash_class_attrib_pseudos())
  //   Visit(hash_class_attrib_pseudo);
}

void Converter::Visit(
    const HashOrClassOrAttribOrPseudo& hash_class_attrib_pseudo) {
  if (hash_class_attrib_pseudo.has_hash()) {
    string_ += "#";
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
  string_ += ".";
  Visit(_class.ident());
}

void Converter::Visit(const Attrib& attrib) {
  string_ += "[";
  Visit(attrib.ident());
  if (attrib.has_attrib_part_two())
    Visit(attrib.attrib_part_two());
  string_ += "]";
}

void Converter::Visit(const AttribPartTwo& attrib_part_two) {
  if (attrib_part_two.equal_includes_dashmatch() == AttribPartTwo::EQUAL)
    string_ += "=";
  else if (attrib_part_two.equal_includes_dashmatch() ==
           AttribPartTwo::INCLUDES)
    string_ += "~=";
  else if (attrib_part_two.equal_includes_dashmatch() ==
           AttribPartTwo::DASHMATCH)
    string_ += "|=";

  if (attrib_part_two.has_ident())
    Visit(attrib_part_two.ident());
  if (attrib_part_two.has_string())
    Visit(attrib_part_two.string());
}

void Converter::Visit(const Pseudo& pseudo) {
  string_ += ":";

  if (pseudo.has_ident_1())
    Visit(pseudo.ident_1());
  else if (pseudo.has_function_token())
    Visit(pseudo.function_token());

  Visit(pseudo.ident_2());
  string_ += ")";
}

void Converter::Visit(const Declaration& declaration) {
  if (declaration.has_nonempty_declaration())
    Visit(declaration.nonempty_declaration());
  // else empty
}

void Converter::Visit(const NonEmptyDeclaration& nonempty_declaration) {
  Visit(nonempty_declaration.property());
  string_ += " : ";
  Visit(nonempty_declaration.expr());
  if (nonempty_declaration.has_prio())
    string_ += " !important";
}

void Converter::Visit(const ElementName& element_name) {
  if (element_name.has_ident())
    Visit(element_name.ident());
  else if (element_name.star())
    string_ += "*";
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
    string_ += "%";
  }
  // S* | LENGTH
  if (term_part.has_length())
    Visit(term_part.length());
  // S* | EMS
  if (term_part.has_ems()) {
    Visit(term_part.ems());
    string_ += "em";
  }
  // S* | EXS
  if (term_part.has_exs()) {
    Visit(term_part.exs());
    string_ += "ex";
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
  string_ += ")";
}

void Converter::Visit(const Hexcolor& hexcolor) {
  string_ += "#";
  Visit(hexcolor.first_three());
  if (hexcolor.has_last_three())
    Visit(hexcolor.last_three());
}

void Converter::Visit(const HexcolorThree& hexcolor_three) {
  string_ += static_cast<char>(hexcolor_three.ascii_value_1());
  string_ += static_cast<char>(hexcolor_three.ascii_value_2());
  string_ += static_cast<char>(hexcolor_three.ascii_value_3());
}

void Converter::Reset() {
  string_ = "";
}
};  // namespace css_parser_proto_fuzzer
