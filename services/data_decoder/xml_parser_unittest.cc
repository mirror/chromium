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

// Parses the passed in |xml| and compares the result to |json|.
// If |json| is empty, the parsing is expected to fail.
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

  EXPECT_FALSE(error) << "Unexpected error: " << *error;
  EXPECT_TRUE(actual_value);

  std::unique_ptr<base::Value> expected_value = base::JSONReader::Read(json);
  DCHECK(expected_value) << "Bad test, incorrect JSON: " << json;

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
  TestParseXml("<a/>", "{\"tag\":\"a\"}");
  TestParseXml("<a><b/></a>", "{\"tag\":\"a\",\"children\":[{\"tag\":\"b\"}]}");
  TestParseXml("<a><b/><b/><b/></a>",
               "{\"tag\":\"a\",\"children\":["
               "   {\"tag\":\"b\"},{\"tag\":\"b\"},{\"tag\":\"b\"}]}");
}

TEST_F(XmlParserTest, ParseEmptyTag) {
  TestParseXml("<a></a>", "{\"tag\":\"a\"}");
  TestParseXml("<a><b></b></a>",
               "{\"tag\":\"a\",\"children\":[{\"tag\":\"b\"}]}");
  TestParseXml("<a><b></b><b></b></a>",
               "{\"tag\":\"a\",\"children\":["
               "  {\"tag\":\"b\"},{\"tag\":\"b\"}]}");
}

TEST_F(XmlParserTest, ParseTextElement) {
  TestParseXml("<hello>bonjour</hello>",
               "{\"tag\":\"hello\",\"text\":\"bonjour\"}");
}

TEST_F(XmlParserTest, ParseMultipleSimilarTextElement) {
  TestParseXml("<hello><fr>bonjour</fr><fr>salut</fr><fr>coucou</fr></hello>",
               "{\"tag\":\"hello\",\"children\":["
               "  {\"tag\":\"fr\",\"text\":\"bonjour\"},"
               "  {\"tag\":\"fr\",\"text\":\"salut\"},"
               "  {\"tag\":\"fr\",\"text\":\"coucou\"}]}");
}

TEST_F(XmlParserTest, ParseMixMatchTextNonTextElement) {
  TestParseXml(
      "<hello>"
      "  <fr>coucou</fr>"
      "  <fr><proper>bonjour</proper><slang>salut</slang></fr>"
      "   <fr>ca va</fr>"
      "</hello>",
      "{\"tag\":\"hello\","
      " \"children\":["
      "    {\"tag\":\"fr\",\"text\":\"coucou\"},"
      "    {\"tag\":\"fr\",\"children\":["
      "        {\"tag\":\"proper\",\"text\":\"bonjour\"},"
      "        {\"tag\":\"slang\",\"text\":\"salut\"}]},"
      "    {\"tag\":\"fr\",\"text\":\"ca va\"}]}");
}

TEST_F(XmlParserTest, ParseNestedXml) {
  TestParseXml(
      "<M><a><t><r><y><o><s><h><k><a>Zdravstvuy"
      "</a></k></h></s></o></y></r></t></a></M>",
      "{\"tag\":\"M\",\"children\":[{\"tag\":\"a\",\"children\":["
      "  {\"tag\":\"t\",\"children\":[{\"tag\":\"r\",\"children\":["
      "    {\"tag\":\"y\",\"children\":[{\"tag\":\"o\",\"children\":["
      "      {\"tag\":\"s\",\"children\":[{\"tag\":\"h\",\"children\":["
      "        {\"tag\":\"k\",\"children\":[{\"tag\":\"a\","
      "                                     \"text\":\"Zdravstvuy\"}]}"
      "      ]}]}"
      "    ]}]}"
      "  ]}]}"
      "]}]}");
}

TEST_F(XmlParserTest, ParseMultipleSimilarElements) {
  TestParseXml("<a><b>b1</b><c>c1</c><b>b2</b><c>c2</c><b>b3</b><c>c3</c></a>",
               "{\"tag\":\"a\",\"children\":["
               "  {\"tag\":\"b\",\"text\":\"b1\"},"
               "  {\"tag\":\"c\",\"text\":\"c1\"},"
               "  {\"tag\":\"b\",\"text\":\"b2\"},"
               "  {\"tag\":\"c\",\"text\":\"c2\"},"
               "  {\"tag\":\"b\",\"text\":\"b3\"},"
               "  {\"tag\":\"c\",\"text\":\"c3\"}"
               "]}");
}

TEST_F(XmlParserTest, ParseAttributes) {
  TestParseXml("<a b=\"c\"/>", "{\"tag\":\"a\",\"attributes\":{\"b\":\"c\"}}");
  TestParseXml("<a><b c=\"d\"/></a>",
               "{\"tag\":\"a\","
               "\"children\":[{\"tag\":\"b\",\"attributes\":{\"c\":\"d\"}}]}");
  TestParseXml("<hello lang=\"fr\">bonjour</hello>",
               "{\"tag\":\"hello\",\"text\":\"bonjour\","
               "\"attributes\":{\"lang\":\"fr\"}}");
  TestParseXml(
      "<translate lang=\"fr\" id=\"123\"><hello>bonjour</hello></translate>",
      "{\"tag\": \"translate\",\"attributes\":{\"lang\":\"fr\",\"id\":\"123\"},"
      "\"children\":[{\"tag\":\"hello\", \"text\":\"bonjour\"}]}");
}

TEST_F(XmlParserTest, MultipleNamespacesDefined) {
  TestParseXml(
      "<a xmlns='http://a' xmlns:foo='http://foo' xmlns:bar='http://bar'></a>",
      "{\"tag\": \"a\", \"namespaces\":{"
      "\"\":\"http://a\",\"foo\":\"http://foo\",\"bar\":\"http://bar\"}}");
}

TEST_F(XmlParserTest, NamespacesUsed) {
  TestParseXml(
      "<foo:a xmlns:foo='http://foo'>"
      "  <foo:b att1='fooless' foo:att2='fooful'>With foo</foo:b>"
      "  <b foo:att1='fooful' att2='fooless'>No foo</b>"
      "</foo:a>",
      "{\"tag\": \"foo:a\","
      " \"namespaces\":{\"foo\":\"http://foo\"},"
      " \"children\":["
      "   {\"tag\":\"foo:b\","
      "    \"text\":\"With foo\","
      "    \"attributes\":{\"att1\":\"fooless\",\"foo:att2\":\"fooful\"}},"
      "   {\"tag\":\"b\","
      "    \"text\":\"No foo\","
      "    \"attributes\":{\"foo:att1\":\"fooful\",\"att2\":\"fooless\"}}"
      " ]}");
}

TEST_F(XmlParserTest, ParseTypicalXml) {
  constexpr char kXml[] =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<!-- This is an XML sample -->"
      "<library xmlns='http://library' xmlns:foo='http://foo.com'>"
      "  <book foo:id=\"k123\">"
      "    <author>Isaac Newton</author>"
      "    <title>Philosophiae Naturalis Principia Mathematica</title>"
      "    <genre>Science</genre>"
      "    <price>40.95</price>"
      "    <publish_date>1947-9-03</publish_date>"
      "  </book>"
      "  <book foo:id=\"k456\">"
      "    <author>Dr. Seuss</author>"
      "    <title>Green Eggs and Ham</title>"
      "    <genre>Kid</genre>"
      "    <foo:kids/>"
      "    <price>4.95</price>"
      "    <publish_date>1960-8-12</publish_date>"
      "  </book>"
      "</library>";

  constexpr char kJson[] =
      "{\"tag\": \"library\","
      " \"namespaces\":{\"\":\"http://library\",\"foo\":\"http://foo.com\"},"
      " \"children\":["
      "  {\"tag\": \"book\","
      "   \"attributes\": {\"foo:id\":\"k123\"},"
      "   \"children\": ["
      "      {\"tag\": \"author\", \"text\": \"Isaac Newton\"},"
      "      {\"tag\": \"title\","
      "       \"text\": \"Philosophiae Naturalis Principia Mathematica\"},"
      "      {\"tag\": \"genre\", \"text\": \"Science\"},"
      "      {\"tag\": \"price\", \"text\": \"40.95\"},"
      "      {\"tag\": \"publish_date\", \"text\": \"1947-9-03\"}"
      "    ]"
      "  },"
      "  {\"tag\": \"book\","
      "   \"attributes\": {\"foo:id\":\"k456\"},"
      "   \"children\": ["
      "      {\"tag\": \"author\", \"text\": \"Dr. Seuss\"},"
      "      {\"tag\": \"title\", \"text\": \"Green Eggs and Ham\"},"
      "      {\"tag\": \"genre\", \"text\": \"Kid\"},"
      "      {\"tag\": \"foo:kids\"},"
      "      {\"tag\": \"price\", \"text\": \"4.95\"},"
      "      {\"tag\": \"publish_date\", \"text\": \"1960-8-12\"}"
      "   ]"
      "  }"
      "]}";
  TestParseXml(kXml, kJson);
}

}  // namespace data_decoder
