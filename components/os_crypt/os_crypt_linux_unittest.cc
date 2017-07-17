// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "components/os_crypt/key_storage_linux.h"
#include "components/os_crypt/os_crypt.h"
#include "components/os_crypt/os_crypt_mocker_linux.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::unique_ptr<KeyStorageLinux> GetNullKeyStorage() {
  return nullptr;
}

class OSCryptLinuxTest : public testing::Test {
 public:
  OSCryptLinuxTest() = default;
  ~OSCryptLinuxTest() override = default;

  void SetUp() override {
    OSCryptMockerLinux::SetUp();
    UseMockKeyStorageForTesting(nullptr, OSCryptLinuxTest::GetKey);
  }

  void TearDown() override { OSCryptMockerLinux::TearDown(); }

 protected:
  void SetEncryptionKey(const std::string& key) {
    *OSCryptLinuxTest::GetKey() = key;
  }

  static std::string* GetKey();

 private:
  DISALLOW_COPY_AND_ASSIGN(OSCryptLinuxTest);
};

std::string* OSCryptLinuxTest::GetKey() {
  static std::string* key = new std::string("something");
  return key;
}

TEST_F(OSCryptLinuxTest, VerifyV0) {
  const std::string originaltext = "hello";
  std::string ciphertext;
  std::string decipheredtext;

  SetEncryptionKey(std::string());
  ciphertext = originaltext;  // No encryption
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &decipheredtext));
  ASSERT_EQ(originaltext, decipheredtext);
}

TEST_F(OSCryptLinuxTest, VerifyV10) {
  const std::string originaltext = "hello";
  std::string ciphertext;
  std::string decipheredtext;

  SetEncryptionKey("peanuts");
  ASSERT_TRUE(OSCrypt::EncryptString(originaltext, &ciphertext));
  SetEncryptionKey("not_peanuts");
  ciphertext = ciphertext.substr(3).insert(0, "v10");
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &decipheredtext));
  ASSERT_EQ(originaltext, decipheredtext);
}

TEST_F(OSCryptLinuxTest, VerifyV11) {
  const std::string originaltext = "hello";
  std::string ciphertext;
  std::string decipheredtext;

  SetEncryptionKey(std::string());
  ASSERT_TRUE(OSCrypt::EncryptString(originaltext, &ciphertext));
  ASSERT_EQ(ciphertext.substr(0, 3), "v11");
  ASSERT_TRUE(OSCrypt::DecryptString(ciphertext, &decipheredtext));
  ASSERT_EQ(originaltext, decipheredtext);
}

TEST_F(OSCryptLinuxTest, IsEncryptionAvailable) {
  EXPECT_TRUE(OSCrypt::IsEncryptionAvailable());
  // Restore default GetKeyStorage and GetPassword functions.
  UseMockKeyStorageForTesting(nullptr, nullptr);
  // Mock only GetKeyStorage function.
  UseMockKeyStorageForTesting(GetNullKeyStorage, nullptr);
  EXPECT_FALSE(OSCrypt::IsEncryptionAvailable());
}

}  // namespace
