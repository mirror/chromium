// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// third_party/WebKit/Source/core/css/parser/CSSParserProtoFuzzer.h
#ifndef CSSParserProtoConverter_h
#define CSSParserProtoConverter_h

#include <string>

#include "third_party/WebKit/Source/core/css/parser/CSS.pb.h"

namespace css_parser_proto_fuzzer {

class Converter {
 public:
  Converter();
  std::string Convert(const StyleSheet&);

 private:
  std::string string_;

  void Visit(const Unicode&);
  void Visit(const Escape&);
  void Visit(const Nmstart&);
  void Visit(const Nmchar&);
  void Visit(const String&);
  void Visit(const StringCharOrQuote&, bool use_single);
  void Visit(const StringChar&);
  void Visit(const Ident&);
  void Visit(const Name&);
  void Visit(const Num&);
  void Visit(const UrlChar&);
  void Visit(const W&);
  void Visit(const UnrepeatedW&);
  void Visit(const Nl&);
  void Visit(const Length&);
  void Visit(const Angle&);
  void Visit(const Time&);
  void Visit(const Freq&);
  void Visit(const Uri&);
  void Visit(const FunctionToken&);
  void Visit(const StyleSheet&);
  void Visit(const CharsetDeclaration&);
  void Visit(const RulesetOrMediaOrPageOrFontFace&);
  void Visit(const Import&);
  void Visit(const MediumList&);
  void Visit(const Namespace&);
  void Visit(const NamespacePrefix&);
  void Visit(const Media&);
  void Visit(const Medium&);
  void Visit(const Page&);
  void Visit(const DeclarationList&);
  void Visit(const PseudoPage&);
  void Visit(const FontFace&);
  void Visit(const Operator&);
  void Visit(const Combinator&);
  void Visit(const UnaryOperator&);
  void Visit(const Property&);
  void Visit(const Ruleset&);
  void Visit(const SelectorList&);
  void Visit(const Selector&);
  void Visit(const CombinatorAndSimpleSelector&);
  void Visit(const SimpleSelector&);
  void Visit(const HashOrClassOrAttribOrPseudo&);
  void Visit(const _Class&);
  void Visit(const ElementName&);
  void Visit(const Attrib&);
  void Visit(const AttribPartTwo&);
  void Visit(const Pseudo&);
  void Visit(const Declaration&);
  void Visit(const NonEmptyDeclaration&);
  void Visit(const Expr&);
  void Visit(const OperatorTerm&);
  void Visit(const Term&);
  void Visit(const TermPart&);
  void Visit(const Function&);
  void Visit(const Hexcolor&);
  void Visit(const HexcolorThree&);

  void Reset();
};
};  // namespace css_parser_proto_fuzzer

#endif  // CSSParserProtoConverter_h
