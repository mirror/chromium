// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"

#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "services/data_decoder/data_decoder_service.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

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
    "<options allowStop=\"false\"/>\n"
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
    "<state></state>\n"
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

class SafeDialAppInfoParserTest : public testing::Test {
 public:
  SafeDialAppInfoParserTest()
      : connector_factory_(
            std::make_unique<data_decoder::DataDecoderService>()) {
    connector_ = connector_factory_.CreateConnector();
  }

  std::unique_ptr<ParsedDialAppInfo> Parse(
      const std::string& xml,
      SafeDialAppInfoParser::ParsingError expected_error) {
    base::RunLoop run_loop;
    SafeDialAppInfoParser parser(connector_.get());
    parser.Parse(xml,
                 base::BindOnce(&SafeDialAppInfoParserTest::OnParsingCompleted,
                                base::Unretained(this), expected_error));
    base::RunLoop().RunUntilIdle();
    return std::move(app_info_);
  }

  void OnParsingCompleted(SafeDialAppInfoParser::ParsingError expected_error,
                          std::unique_ptr<ParsedDialAppInfo> app_info,
                          SafeDialAppInfoParser::ParsingError error) {
    app_info_ = std::move(app_info);
    EXPECT_EQ(expected_error, error);
  }

 private:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<service_manager::Connector> connector_;
  service_manager::TestConnectorFactory connector_factory_;
  std::unique_ptr<ParsedDialAppInfo> app_info_;
  DISALLOW_COPY_AND_ASSIGN(SafeDialAppInfoParserTest);
};

TEST_F(SafeDialAppInfoParserTest, TestInvalidXmlNoService) {
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse("", SafeDialAppInfoParser::ParsingError::kInvalidXML);
  EXPECT_FALSE(app_info);
}

TEST_F(SafeDialAppInfoParserTest, TestValidXml) {
  std::string xml_text(kValidAppInfoXml);
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse(xml_text, SafeDialAppInfoParser::ParsingError::kNone);

  EXPECT_EQ("YouTube", app_info->name);
  EXPECT_EQ(DialAppState::RUNNING, app_info->state);
  EXPECT_FALSE(app_info->allow_stop);
  EXPECT_EQ("run", app_info->href);
  EXPECT_FALSE(app_info->extra_data);
}

TEST_F(SafeDialAppInfoParserTest, TestValidXmlExtraData) {
  std::string xml_text(kValidAppInfoXmlExtraData);
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse(xml_text, SafeDialAppInfoParser::ParsingError::kNone);

  EXPECT_EQ("YouTube", app_info->name);
  EXPECT_EQ(DialAppState::RUNNING, app_info->state);
  EXPECT_TRUE(app_info->extra_data);

  const base::Value* capability_element =
      app_info->extra_data->FindKey("capabilities");
  std::string capabilities;
  EXPECT_TRUE(
      data_decoder::GetXmlElementText(*capability_element, &capabilities));
  EXPECT_EQ("websocket", capabilities);
}

TEST_F(SafeDialAppInfoParserTest, TestInvalidXmlNoState) {
  std::string xml_text(kInvalidXmlNoState);
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse(xml_text, SafeDialAppInfoParser::ParsingError::kFailToReadState);
  EXPECT_FALSE(app_info);
}

TEST_F(SafeDialAppInfoParserTest, TestInvalidXmlInvalidState) {
  std::string xml_text(kInvalidXmlInvalidState);
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse(xml_text, SafeDialAppInfoParser::ParsingError::kInvalidState);
  EXPECT_FALSE(app_info);
}

TEST_F(SafeDialAppInfoParserTest, TestInvalidXmlNoName) {
  std::string xml_text(kInvalidXmlNoName);
  std::unique_ptr<ParsedDialAppInfo> app_info =
      Parse(xml_text, SafeDialAppInfoParser::ParsingError::kMissingName);
  EXPECT_FALSE(app_info);
}

}  // namespace media_router
