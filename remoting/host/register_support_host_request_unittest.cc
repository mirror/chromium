// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/register_support_host_request.h"

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "remoting/base/constants.h"
#include "remoting/host/host_key_pair.h"
#include "remoting/host/in_memory_host_config.h"
#include "remoting/host/test_key_pair.h"
#include "remoting/jingle_glue/iq_sender.h"
#include "remoting/jingle_glue/mock_objects.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle/source/talk/xmllite/xmlelement.h"
#include "third_party/libjingle/source/talk/xmpp/constants.h"

using buzz::QName;
using buzz::XmlElement;

using testing::_;
using testing::Invoke;
using testing::NotNull;
using testing::Return;
using testing::SaveArg;

namespace remoting {

namespace {
const char kTestJid[] = "user@gmail.com/chromoting123";
const int64 kTestTime = 123123123;
const char kSupportId[] = "AB4RF3";
const char kSupportIdLifetime[] = "300";
const char kStanzaId[] = "123";

class MockCallback {
 public:
  MOCK_METHOD3(OnResponse, void(bool result, const std::string& support_id,
                                const base::TimeDelta& lifetime));
};

}  // namespace

class RegisterSupportHostRequestTest : public testing::Test {
 public:
 protected:
  virtual void SetUp() {
    config_ = new InMemoryHostConfig();
    config_->SetString(kPrivateKeyConfigPath, kTestHostKeyPair);
  }

  MockSignalStrategy signal_strategy_;
  MessageLoop message_loop_;
  scoped_refptr<InMemoryHostConfig> config_;
  MockCallback callback_;
};

TEST_F(RegisterSupportHostRequestTest, Send) {
  // |iq_request| is freed by RegisterSupportHostRequest.
  int64 start_time = static_cast<int64>(base::Time::Now().ToDoubleT());

  SignalStrategy::Listener* listener;
  EXPECT_CALL(signal_strategy_, AddListener(NotNull()))
      .WillOnce(SaveArg<0>(&listener));

  scoped_ptr<RegisterSupportHostRequest> request(
      new RegisterSupportHostRequest());
  ASSERT_TRUE(request->Init(
      config_, base::Bind(&MockCallback::OnResponse,
                          base::Unretained(&callback_))));

  XmlElement* sent_iq = NULL;
  EXPECT_CALL(signal_strategy_, GetNextId())
      .WillOnce(Return(kStanzaId));
  EXPECT_CALL(signal_strategy_, SendStanza(NotNull()))
      .WillOnce(DoAll(SaveArg<0>(&sent_iq), Return(true)));

  request->OnSignallingConnected(&signal_strategy_, kTestJid);
  message_loop_.RunAllPending();

  // Verify format of the query.
  scoped_ptr<XmlElement> stanza(sent_iq);
  ASSERT_TRUE(stanza != NULL);

  EXPECT_EQ(stanza->Attr(buzz::QName("", "to")),
            std::string(kChromotingBotJid));
  EXPECT_EQ(stanza->Attr(buzz::QName("", "type")), "set");

  EXPECT_EQ(QName(kChromotingXmlNamespace, "register-support-host"),
            stanza->FirstElement()->Name());

  QName signature_tag(kChromotingXmlNamespace, "signature");
  XmlElement* signature = stanza->FirstElement()->FirstNamed(signature_tag);
  ASSERT_TRUE(signature != NULL);
  EXPECT_TRUE(stanza->NextNamed(signature_tag) == NULL);

  std::string time_str =
      signature->Attr(QName(kChromotingXmlNamespace, "time"));
  int64 time;
  EXPECT_TRUE(base::StringToInt64(time_str, &time));
  int64 now = static_cast<int64>(base::Time::Now().ToDoubleT());
  EXPECT_LE(start_time, time);
  EXPECT_GE(now, time);

  HostKeyPair key_pair;
  key_pair.LoadFromString(kTestHostKeyPair);
  std::string expected_signature =
      key_pair.GetSignature(std::string(kTestJid) + ' ' + time_str);
  EXPECT_EQ(expected_signature, signature->BodyText());

  // Generate response and verify that callback is called.
  EXPECT_CALL(callback_, OnResponse(true, kSupportId,
                                    base::TimeDelta::FromSeconds(300)));

  scoped_ptr<XmlElement> response(new XmlElement(buzz::QN_IQ));
  response->AddAttr(QName("", "type"), "result");
  response->AddAttr(QName("", "id"), kStanzaId);

  XmlElement* result = new XmlElement(
      QName(kChromotingXmlNamespace, "register-support-host-result"));
  response->AddElement(result);

  XmlElement* support_id = new XmlElement(
      QName(kChromotingXmlNamespace, "support-id"));
  support_id->AddText(kSupportId);
  result->AddElement(support_id);

  XmlElement* support_id_lifetime = new XmlElement(
      QName(kChromotingXmlNamespace, "support-id-lifetime"));
  support_id_lifetime->AddText(kSupportIdLifetime);
  result->AddElement(support_id_lifetime);

  EXPECT_TRUE(listener->OnIncomingStanza(response.get()));
  message_loop_.RunAllPending();

  EXPECT_CALL(signal_strategy_, RemoveListener(listener));
}

}  // namespace remoting
