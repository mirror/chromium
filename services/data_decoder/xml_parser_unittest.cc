// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/message_loop/message_loop.h"
#include "services/data_decoder/xml_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_decoder {

namespace {

void TestParseXmlCallback(std::unique_ptr<base::Value>* value_out,
                          base::Optional<std::string>* error_out,
                          std::unique_ptr<base::Value> value,
                          const base::Optional<std::string>& error) {
  *value_out = std::move(value);
  *error_out = error;
}

// Parses the passed in |xml| and compare the result to |json|.
// If |json| is empty, it expects the parsing to fail.
void TestParseXml(const std::string& xml, const std::string& json) {
  XmlParser parser_impl(/*service_ref=*/nullptr);
  // Use a reference to mojom::XmlParser as XmlParser implements the interface
  // privately.
  mojom::XmlParser& parser = parser_impl;

  std::unique_ptr<base::Value> actual_value;
  base::Optional<std::string> error;
  parser.Parse(xml, base::Bind(&TestParseXmlCallback, &actual_value, &error));
  if (json.empty()) {
    EXPECT_TRUE(error);
    EXPECT_FALSE(actual_value);
    return;
  }

  EXPECT_FALSE(error);
  EXPECT_TRUE(actual_value);

  std::unique_ptr<base::Value> expected_value = base::JSONReader::Read(json);
  DCHECK(expected_value);

  EXPECT_EQ(*expected_value, *actual_value);
}

}  // namespace

using XmlParserTest = testing::Test;

TEST_F(XmlParserTest, ParseBadXml) {
  std::string invalid_xml_strings[] = {"",
                                       "  ",
                                       "Awesome possum",
                                       "[\"json\", \"or\", \"xml?\"]",
                                       "<unbalanced>",
                                       "<hello>bad tag</goodbye>"};
  for (auto& xml : invalid_xml_strings)
    TestParseXml(xml, "");
}

TEST_F(XmlParserTest, ParseSelfClosingTag) {
  TestParseXml("<a/>", "{\"a\":{}}");
  TestParseXml("<a><b/></a>", "{\"a\":{\"b\":{}}}");
  TestParseXml("<a><b/><b/><b/></a>", "{\"a\":{\"b\":[{},{},{}]}}");
}

TEST_F(XmlParserTest, ParseEmptyTag) {
  TestParseXml("<a></a>", "{\"a\":{}}");
  TestParseXml("<a><b></b></a>", "{\"a\":{\"b\":{}}}");
}

TEST_F(XmlParserTest, ParseMultipleEmptyTag) {
  TestParseXml("<a><b></b><b></b></a>", "{\"a\":{\"b\":[{},{}]}}");
}

TEST_F(XmlParserTest, ParseTextElement) {
  TestParseXml("<hello>bonjour</hello>", "{\"hello\":\"bonjour\"}");
}

TEST_F(XmlParserTest, ParseMultipleSimilarTextElement) {
  TestParseXml("<hello><fr>bonjour</fr><fr>salut</fr><fr>coucou</fr></hello>",
               "{\"hello\":{\"fr\":[\"bonjour\",\"salut\",\"coucou\"]}}");
}

TEST_F(XmlParserTest, ParseMixMatchTextNonTextElement) {
  TestParseXml(
      "<hello><fr>coucou</fr><fr><proper>bonjour</proper><slang>salut</slang></"
      "fr><fr>ca va</fr></hello>",
      "{\"hello\":{\"fr\":[\"coucou\",{\"proper\":\"bonjour\",\"slang\":"
      "\"salut\"},\"ca va\"]}}");
}

TEST_F(XmlParserTest, ParseNestedXml) {
  TestParseXml(
      "<M><a><t><r><y><o><s><h><k><a>Zdravstvuy"
      "</a></k></h></s></o></y></r></t></a></M>",
      "{\"M\":{\"a\":{\"t\":{\"r\":{\"y\":{\"o\":{\"s\":{\"h\":{\"k\":{\"a\":"
      "\"Zdravstvuy\"}}}}}}}}}}");
}

TEST_F(XmlParserTest, ParseMultipleSimilarElements) {
  TestParseXml(
      "<a><b>b1</b><c>c1</c><b>b2</b><c>c2</c><b>b3</b><c>c3</c></a>",
      "{\"a\":{\"b\":[\"b1\",\"b2\",\"b3\"],\"c\":[\"c1\",\"c2\",\"c3\"]}}");
}

TEST_F(XmlParserTest, ParseTypicalXml) {
  constexpr char kXml[] =
      "<!-- This is an XML sample -->"
      "<library>"
      "  <book id=\"k123\">"
      "    <author>Isaac Newton</author>"
      "    <title>Philosophiae Naturalis Principia Mathematica</title>"
      "    <genre>Science</genre>"
      "    <price>40.95</price>"
      "    <publish_date>1947-9-03</publish_date>"
      "  </book>"
      "  <book id=\"k456\">"
      "    <author>Dr. Seuss</author>"
      "    <title>Green Eggs and Ham</title>"
      "    <genre>Kid</genre>"
      "    <kids/>"
      "    <price>4.95</price>"
      "    <publish_date>1960-8-12</publish_date>"
      "  </book>"
      "</library>";

  constexpr char kJson[] =
      "{\"library\":"
      "  {\"book\":"
      "   [{"
      "      \"author\":\"Isaac Newton\","
      "      \"title\":\"Philosophiae Naturalis Principia Mathematica\","
      "      \"genre\":\"Science\","
      "      \"price\":\"40.95\","
      "      \"publish_date\":\"1947-9-03\""
      "    },"
      "    {"
      "      \"author\":\"Dr. Seuss\","
      "      \"title\":\"Green Eggs and Ham\","
      "      \"genre\":\"Kid\","
      "      \"price\":\"4.95\","
      "      \"publish_date\":\"1960-8-12\","
      "      \"kids\":{}"
      "    }"
      "   ]"
      "  }"
      "}";
  TestParseXml(kXml, kJson);
}

}  // namespace data_decoder
