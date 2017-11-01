// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/utility/media_router/dial_app_info_parser_impl.h"

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kValidAppInfoXml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\">\n"
    "<name>YouTube</name>\n"
    "<options allowStop=\"false\"/>\n"
    "<state>running</state>\n"
    "<link rel=\"run\" href=\"run\"/>\n"
    "</service>";

constexpr char kValidAppInfoXmlExtraData[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\">\n"
    "<name>YouTube</name>\n"
    "<state>running</state>\n"
    "<link rel=\"run\" href=\"run\"/>\n"
    "<port>8080</port>\n"
    "<capabilities>websocket</capabilities>\n"
    "<additionalData>\n"
    "<screenId>e5n3112oskr42pg0td55b38nh4</screenId>\n"
    "<otherField>2</otherField>\n"
    "</additionalData>\n"
    "</service>";

constexpr char kInvalidXmlNoState[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\">\n"
    "<name>YouTube</name>\n"
    "<options allowStop=\"false\"/>\n"
    "<link rel=\"run\" href=\"run\"/>\n"
    "</service>";

constexpr char kInvalidXmlInvalidState[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\">\n"
    "<name>YouTube</name>\n"
    "<options allowStop=\"false\"/>\n"
    "<state>xyzzy</state>\n"
    "<link rel=\"run\" href=\"run\"/>\n"
    "</service>";

constexpr char kInvalidXmlNoName[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\">\n"
    "<options allowStop=\"false\"/>\n"
    "<state>running</state>\n"
    "<link rel=\"run\" href=\"run\"/>\n"
    "</service>";

}  // namespace

namespace media_router {

class DialAppInfoParserImplTest : public testing::Test {
 public:
  DialAppInfoParserImplTest() {}
  chrome::mojom::DialAppInfoPtr Parse(
      const std::string& xml,
      chrome::mojom::DialAppInfoParsingError expected_error) {
    chrome::mojom::DialAppInfoParsingError error;
    auto out = parser_.Parse(xml, &error);
    EXPECT_EQ(expected_error, error);
    return out;
  }

 private:
  DialAppInfoParserImpl parser_;
  DISALLOW_COPY_AND_ASSIGN(DialAppInfoParserImplTest);
};

TEST_F(DialAppInfoParserImplTest, TestInvalidXmlNoService) {
  chrome::mojom::DialAppInfoPtr app_info =
      Parse("", chrome::mojom::DialAppInfoParsingError::INVALID_SERVICE);
  ASSERT_FALSE(app_info);
}

TEST_F(DialAppInfoParserImplTest, TestValidXml) {
  std::string xml_text(kValidAppInfoXml);
  chrome::mojom::DialAppInfoPtr app_info =
      Parse(xml_text, chrome::mojom::DialAppInfoParsingError::NONE);

  ASSERT_TRUE(app_info);
  EXPECT_EQ("YouTube", app_info->name);
  EXPECT_EQ(chrome::mojom::DialAppState::RUNNING, app_info->state);
  EXPECT_FALSE(app_info->capabilities.has_value());
}

TEST_F(DialAppInfoParserImplTest, TestValidXmlExtraData) {
  std::string xml_text(kValidAppInfoXmlExtraData);
  chrome::mojom::DialAppInfoPtr app_info =
      Parse(xml_text, chrome::mojom::DialAppInfoParsingError::NONE);

  ASSERT_TRUE(app_info);
  EXPECT_EQ("YouTube", app_info->name);
  EXPECT_EQ(chrome::mojom::DialAppState::RUNNING, app_info->state);
  EXPECT_EQ("websocket", app_info->capabilities.value());
}

TEST_F(DialAppInfoParserImplTest, TestInvalidXmlNoState) {
  std::string xml_text(kInvalidXmlNoState);
  chrome::mojom::DialAppInfoPtr app_info =
      Parse(xml_text, chrome::mojom::DialAppInfoParsingError::
                          GET_RESPONSE_HAS_INVALID_STATE_VALUE);
  ASSERT_FALSE(app_info);
}

TEST_F(DialAppInfoParserImplTest, TestInvalidXmlInvalidState) {
  std::string xml_text(kInvalidXmlInvalidState);
  chrome::mojom::DialAppInfoPtr app_info =
      Parse(xml_text, chrome::mojom::DialAppInfoParsingError::
                          GET_RESPONSE_HAS_INVALID_STATE_VALUE);
  ASSERT_FALSE(app_info);
}

TEST_F(DialAppInfoParserImplTest, TestInvalidXmlNoName) {
  std::string xml_text(kInvalidXmlNoName);
  chrome::mojom::DialAppInfoPtr app_info = Parse(
      xml_text,
      chrome::mojom::DialAppInfoParsingError::GET_RESPONSE_MISSING_NAME_VALUE);
  ASSERT_FALSE(app_info);
}

}  // namespace media_router
