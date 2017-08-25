// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/ComponentUpdater_jni.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_path_override.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/version.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/component_updater_service_internal.h"
#include "components/component_updater/default_component_installer.h"
#include "components/update_client/component_unpacker.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/test_configurator.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ComponentUnpacker = update_client::ComponentUnpacker;
using Configurator = update_client::Configurator;
using CrxUpdateItem = update_client::CrxUpdateItem;
using TestConfigurator = update_client::TestConfigurator;
using UpdateClient = update_client::UpdateClient;

using ::testing::_;
using ::testing::Invoke;

using base::android::GetClass;
using base::android::ConvertUTF8ToJavaString;
using base::android::ClearException;

namespace component_updater {

namespace {

// This hash corresponds to jebgalgnebhfojomionfpkfelancnnkf.crx.
const uint8_t kSha256Hash[] = {0x94, 0x16, 0x0b, 0x6d, 0x41, 0x75, 0xe9, 0xec,
                               0x8e, 0xd5, 0xfa, 0x54, 0xb0, 0xd2, 0xdd, 0xa5,
                               0x6e, 0x05, 0x6b, 0xe8, 0x73, 0x47, 0xf6, 0xc4,
                               0x11, 0x9f, 0xbc, 0xb3, 0x09, 0xb3, 0x5b, 0x40};

constexpr base::FilePath::CharType relative_install_dir[] =
    FILE_PATH_LITERAL("fake");

base::FilePath test_file(const char* file) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  return path.AppendASCII("components")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("update_client")
      .AppendASCII(file);
}

base::FilePath apk_test_file(const char* file) {
  return test_file("apk").AppendASCII(file);
}

class MockUpdateClient : public UpdateClient {
 public:
  MockUpdateClient() {}
  MOCK_METHOD1(AddObserver, void(Observer* observer));
  MOCK_METHOD1(RemoveObserver, void(Observer* observer));
  MOCK_METHOD3(Install,
               void(const std::string& id,
                    const CrxDataCallback& crx_data_callback,
                    const Callback& callback));
  MOCK_METHOD3(Update,
               void(const std::vector<std::string>& ids,
                    const CrxDataCallback& crx_data_callback,
                    const Callback& callback));
  MOCK_CONST_METHOD2(GetCrxUpdateState,
                     bool(const std::string& id, CrxUpdateItem* update_item));
  MOCK_CONST_METHOD1(IsUpdating, bool(const std::string& id));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD4(SendUninstallPing,
               void(const std::string& id,
                    const base::Version& version,
                    int reason,
                    const Callback& callback));

 private:
  ~MockUpdateClient() override {}
};

class FakeInstallerTraits : public ComponentInstallerTraits {
 public:
  FakeInstallerTraits() {}
  ~FakeInstallerTraits() override {}

  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& dir) const override {
    return true;
  }

  bool SupportsGroupPolicyEnabledComponentUpdates() const override {
    return true;
  }

  bool RequiresNetworkEncryption() const override { return true; }

  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictionaryValue& manifest,
      const base::FilePath& install_dir) override {
    return update_client::CrxInstaller::Result(0);
  }

  void ComponentReady(
      const base::Version& version,
      const base::FilePath& install_dir,
      std::unique_ptr<base::DictionaryValue> manifest) override {}

  base::FilePath GetRelativeInstallDir() const override {
    return base::FilePath(relative_install_dir);
  }

  void GetHash(std::vector<uint8_t>* hash) const override { GetPkHash(hash); }

  std::string GetName() const override { return "fake name"; }

  update_client::InstallerAttributes GetInstallerAttributes() const override {
    update_client::InstallerAttributes installer_attributes;
    installer_attributes["ap"] = "fake-ap";
    installer_attributes["is-enterprise"] = "1";
    return installer_attributes;
  }

  std::vector<std::string> GetMimeTypes() const override {
    return std::vector<std::string>();
  }

 private:
  static void GetPkHash(std::vector<uint8_t>* hash) {
    hash->assign(std::begin(kSha256Hash), std::end(kSha256Hash));
  }
};

class DefaultComponentInstallerTest : public testing::Test {
 public:
  DefaultComponentInstallerTest();
  ~DefaultComponentInstallerTest() override;

  MockUpdateClient& update_client() { return *update_client_; }
  ComponentUpdateService* component_updater() {
    return component_updater_.get();
  }
  scoped_refptr<TestConfigurator> configurator() const { return config_; }
  base::Closure quit_closure() const { return quit_closure_; }

 protected:
  void RunThreads();
  void Unpack(const base::FilePath& crx_path);
  ComponentUnpacker::Result result() const { return result_; }

 private:
  void UnpackComplete(const ComponentUnpacker::Result& result);

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_ =
      base::ThreadTaskRunnerHandle::Get();
  base::RunLoop runloop_;
  base::Closure quit_closure_ = runloop_.QuitClosure();

  scoped_refptr<TestConfigurator> config_ =
      base::MakeRefCounted<TestConfigurator>();
  scoped_refptr<MockUpdateClient> update_client_ =
      base::MakeRefCounted<MockUpdateClient>();
  std::unique_ptr<ComponentUpdateService> component_updater_;
  ComponentUnpacker::Result result_;
};

DefaultComponentInstallerTest::DefaultComponentInstallerTest() {
  EXPECT_CALL(update_client(), AddObserver(_)).Times(1);
  component_updater_ =
      base::MakeUnique<CrxUpdateService>(config_, update_client_);
}

DefaultComponentInstallerTest::~DefaultComponentInstallerTest() {
  EXPECT_CALL(update_client(), RemoveObserver(_)).Times(1);
  component_updater_.reset();
}

void DefaultComponentInstallerTest::RunThreads() {
  runloop_.Run();
}

void DefaultComponentInstallerTest::Unpack(const base::FilePath& crx_path) {
  auto component_unpacker = base::MakeRefCounted<ComponentUnpacker>(
      std::vector<uint8_t>(std::begin(kSha256Hash), std::end(kSha256Hash)),
      crx_path, nullptr, nullptr);
  component_unpacker->Unpack(base::Bind(
      &DefaultComponentInstallerTest::UnpackComplete, base::Unretained(this)));
  RunThreads();
}

void DefaultComponentInstallerTest::UnpackComplete(
    const ComponentUnpacker::Result& result) {
  result_ = result;

  EXPECT_EQ(update_client::UnpackerError::kNone, result_.error);
  EXPECT_EQ(0, result_.extended_error);

  main_thread_task_runner_->PostTask(FROM_HERE, quit_closure_);
}

}  // namespace

TEST_F(DefaultComponentInstallerTest, LoadPublicKeyFromX509Cert) {
  JNIEnv* env = base::android::AttachCurrentThread();

  auto cert1_path =
      ConvertUTF8ToJavaString(env, apk_test_file("1.crt").MaybeAsASCII());
  auto cert2_path =
      ConvertUTF8ToJavaString(env, apk_test_file("2.crt").MaybeAsASCII());

  // Loading a public key from a valid certificate doesn't throw an exception.

  Java_ComponentUpdater_loadPublicKey(env, cert1_path);
  EXPECT_FALSE(ClearException(env));

  Java_ComponentUpdater_loadPublicKey(env, cert2_path);
  EXPECT_FALSE(ClearException(env));

  // Trying to open a file that doesn't exist causes a FileNotFoundException to
  // be thrown.
  auto file_not_found_exception =
      GetClass(env, "java/io/FileNotFoundException");
  Java_ComponentUpdater_loadPublicKey(
      env, ConvertUTF8ToJavaString(env, "nonexistent"));
  auto* exception = env->ExceptionOccurred();
  EXPECT_TRUE(ClearException(env));
  EXPECT_TRUE(env->IsInstanceOf(exception, file_not_found_exception.obj()));
}

TEST_F(DefaultComponentInstallerTest, VerifyAPKSignature) {
  JNIEnv* env = base::android::AttachCurrentThread();

  auto cert1_path =
      ConvertUTF8ToJavaString(env, apk_test_file("1.crt").MaybeAsASCII());
  auto cert2_path =
      ConvertUTF8ToJavaString(env, apk_test_file("2.crt").MaybeAsASCII());

  // An APK signed with certificate #1.
  auto signed1_apk = ConvertUTF8ToJavaString(
      env, apk_test_file("signed-1.apk").MaybeAsASCII());
  // Signed with #2.
  auto signed2_apk = ConvertUTF8ToJavaString(
      env, apk_test_file("signed-2.apk").MaybeAsASCII());
  // Signed with both certificates.
  auto signed_both_apk = ConvertUTF8ToJavaString(
      env, apk_test_file("signed-1-2.apk").MaybeAsASCII());

  auto unsigned_apk = ConvertUTF8ToJavaString(
      env, apk_test_file("unsigned.apk").MaybeAsASCII());

  auto pk1 = Java_ComponentUpdater_loadPublicKey(env, cert1_path);
  auto pk2 = Java_ComponentUpdater_loadPublicKey(env, cert2_path);

  // Signed APKs should verify with the corresponding public key.
  Java_ComponentUpdater_verify(env, signed1_apk, pk1);
  EXPECT_FALSE(ClearException(env));

  Java_ComponentUpdater_verify(env, signed2_apk, pk2);
  EXPECT_FALSE(ClearException(env));

  // APKs signed with multiple certificates should verify with either
  // corresponding public key.
  Java_ComponentUpdater_verify(env, signed_both_apk, pk1);
  EXPECT_FALSE(ClearException(env));

  Java_ComponentUpdater_verify(env, signed_both_apk, pk2);
  EXPECT_FALSE(ClearException(env));

  // InvalidKeyException should be thrown when none of the signing certificates
  // match the given public key.
  auto invalid_key_exception =
      GetClass(env, "java/security/InvalidKeyException");
  // ApkSignatureSchemeV2Verifier.SignatureNotFoundException should be thrown
  // when an APK isn't signed at all.
  auto signature_not_found_exception =
      GetClass(env,
               "org/chromium/third_party/android/util/apk/"
               "ApkSignatureSchemeV2Verifier$SignatureNotFoundException");

  {
    // Signed APKs shouldn't verify given the wrong public key.
    Java_ComponentUpdater_verify(env, signed1_apk, pk2);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(env->IsInstanceOf(exception, invalid_key_exception.obj()));
  }

  {
    Java_ComponentUpdater_verify(env, signed2_apk, pk1);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(env->IsInstanceOf(exception, invalid_key_exception.obj()));
  }

  {
    // Unsigned APKs shouldn't verify.
    Java_ComponentUpdater_verify(env, unsigned_apk, pk1);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(
        env->IsInstanceOf(exception, signature_not_found_exception.obj()));
  }
}

// Tests that the component metadata is propagated from the default
// component installer and its component traits, through the instance of the
// CrxComponent, to the component updater service.
TEST_F(DefaultComponentInstallerTest, RegisterComponent) {
  class LoopHandler {
   public:
    LoopHandler(int max_cnt, const base::Closure& quit_closure)
        : max_cnt_(max_cnt), quit_closure_(quit_closure) {}

    void OnUpdate(const std::vector<std::string>& ids,
                  const UpdateClient::CrxDataCallback& crx_data_callback,
                  const Callback& callback) {
      callback.Run(update_client::Error::NONE);
      static int cnt = 0;
      ++cnt;
      if (cnt >= max_cnt_)
        quit_closure_.Run();
    }

   private:
    const int max_cnt_;
    base::Closure quit_closure_;
  };

  base::ScopedPathOverride scoped_path_override(DIR_COMPONENT_USER);

  const std::string id("jebgalgnebhfojomionfpkfelancnnkf");

  // Quit after one update check has been fired.
  LoopHandler loop_handler(1, quit_closure());
  EXPECT_CALL(update_client(), Update(_, _, _))
      .WillRepeatedly(Invoke(&loop_handler, &LoopHandler::OnUpdate));

  EXPECT_CALL(update_client(), GetCrxUpdateState(id, _)).Times(1);
  EXPECT_CALL(update_client(), Stop()).Times(1);

  auto installer = base::MakeRefCounted<DefaultComponentInstaller>(
      base::MakeUnique<FakeInstallerTraits>());
  installer->Register(component_updater(), base::Closure());

  RunThreads();

  CrxUpdateItem item;
  EXPECT_TRUE(component_updater()->GetComponentDetails(id, &item));
  const CrxComponent& component(item.component);

  update_client::InstallerAttributes expected_attrs;
  expected_attrs["ap"] = "fake-ap";
  expected_attrs["is-enterprise"] = "1";

  EXPECT_EQ(
      std::vector<uint8_t>(std::begin(kSha256Hash), std::end(kSha256Hash)),
      component.pk_hash);
  EXPECT_EQ(base::Version("0.0.0.0"), component.version);
  EXPECT_TRUE(component.fingerprint.empty());
  EXPECT_STREQ("fake name", component.name.c_str());
  EXPECT_EQ(expected_attrs, component.installer_attributes);
  EXPECT_TRUE(component.requires_network_encryption);
  EXPECT_TRUE(component.supports_group_policy_enable_component_updates);
}

// Tests that the unpack path is removed when the install succeeded.
TEST_F(DefaultComponentInstallerTest, UnpackPathInstallSuccess) {
  auto installer = base::MakeRefCounted<DefaultComponentInstaller>(
      base::MakeUnique<FakeInstallerTraits>());

  Unpack(test_file("jebgalgnebhfojomionfpkfelancnnkf.crx"));

  const auto unpack_path = result().unpack_path;
  EXPECT_TRUE(base::DirectoryExists(unpack_path));

  auto manifest = update_client::ReadManifest(unpack_path);

  base::ScopedPathOverride scoped_path_override(DIR_COMPONENT_USER);
  base::FilePath base_dir;
  EXPECT_TRUE(PathService::Get(DIR_COMPONENT_USER, &base_dir));
  base_dir = base_dir.Append(relative_install_dir);
  EXPECT_TRUE(base::CreateDirectory(base_dir));
  const auto result = installer->Install(std::move(manifest), unpack_path);
  EXPECT_EQ(0, result.error);
  EXPECT_FALSE(base::PathExists(unpack_path));

  EXPECT_CALL(update_client(), Stop()).Times(1);
}

// Tests that the unpack path is removed when the install failed.
TEST_F(DefaultComponentInstallerTest, UnpackPathInstallError) {
  auto installer = base::MakeRefCounted<DefaultComponentInstaller>(
      base::MakeUnique<FakeInstallerTraits>());

  Unpack(test_file("jebgalgnebhfojomionfpkfelancnnkf.crx"));

  const auto unpack_path = result().unpack_path;
  EXPECT_TRUE(base::DirectoryExists(unpack_path));

  auto manifest = update_client::ReadManifest(unpack_path);

  // Test the precondition that DIR_COMPONENT_USER is not registered with
  // the path service.
  base::FilePath base_dir;
  EXPECT_FALSE(PathService::Get(DIR_COMPONENT_USER, &base_dir));

  // Calling |Install| fails since DIR_COMPONENT_USER does not exist.
  const auto result = installer->Install(std::move(manifest), unpack_path);
  EXPECT_EQ(
      static_cast<int>(update_client::InstallError::NO_DIR_COMPONENT_USER),
      result.error);
  EXPECT_FALSE(base::PathExists(unpack_path));

  EXPECT_CALL(update_client(), Stop()).Times(1);
}

}  // namespace component_updater
