// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/password_manager_porter.h"

#include "build/build_config.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/passwords/password_manager_presenter.h"
#include "chrome/browser/ui/passwords/password_ui_view.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/shell_dialogs/select_file_dialog_factory.h"

namespace {
class TestSelectFileDialogFactory : public ui::SelectFileDialogFactory {
 public:
  ui::SelectFileDialog* Create(
      ui::SelectFileDialog::Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy) override {
    return new TestSelectFileDialog(listener,
                                    std::make_unique<TestSelectFilePolicy>());
  }

 private:
  class TestSelectFilePolicy : public ui::SelectFilePolicy {
   public:
    bool CanOpenSelectFileDialog() override { return true; }
    void SelectFileDenied() override {}
  };

  class TestSelectFileDialog : public ui::SelectFileDialog {
   public:
    TestSelectFileDialog(Listener* listener,
                         std::unique_ptr<ui::SelectFilePolicy> policy)
        : ui::SelectFileDialog(listener, std::move(policy)) {}

   protected:
    void SelectFileImpl(Type type,
                        const base::string16& title,
                        const base::FilePath& default_path,
                        const FileTypeInfo* file_types,
                        int file_type_index,
                        const base::FilePath::StringType& default_extension,
                        gfx::NativeWindow owning_window,
                        void* params) override {
      listener_->FileSelected(default_path, file_type_index, params);
    }
    bool IsRunning(gfx::NativeWindow owning_window) const override {
      return false;
    }
    void ListenerDestroyed() override {}
    bool HasMultipleFileTypeChoicesImpl() override { return false; }
    ~TestSelectFileDialog() override {}
  };
};

class MockPasswordManagerPresenter : public PasswordManagerPresenter {
 public:
  explicit MockPasswordManagerPresenter(PasswordUIView* handler)
      : PasswordManagerPresenter(handler) {}

  ~MockPasswordManagerPresenter() override {}

  std::vector<std::unique_ptr<autofill::PasswordForm>> GetAllPasswords()
      override {
    return std::vector<std::unique_ptr<autofill::PasswordForm>>();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPasswordManagerPresenter);
};

class MockPasswordManagerPorter : public PasswordManagerPorter {
 public:
  MockPasswordManagerPorter(
      MockPasswordManagerPresenter* password_manager_presenter)
      : PasswordManagerPorter(password_manager_presenter) {}

  void ImportPasswordsFromPath(const base::FilePath& path) override {
    // Tests for actually importing passwords are contained in
    // |PasswordImporter|.
    passwordsImported = true;
  }

  void ExportPasswordsToPath(const base::FilePath& path) override {
    // Tests for actually exporting passwords are contained in
    // |PasswordExporter|.
    passwordsExported = true;
  }

  // Flags for keeping track of whether passwords have been imported and
  // exported.
  bool passwordsImported = false;
  bool passwordsExported = false;
};

class MockWebContents : public content::TestWebContents {};

class MockPasswordUIView : public PasswordUIView {
 public:
  MockPasswordUIView() : password_manager_presenter_(this) {
    profile_.reset(new TestingProfile());
    password_manager_presenter_.Initialize();
  }
  ~MockPasswordUIView() override {}
  Profile* GetProfile() override { return profile_.get(); }

  void ShowPassword(size_t,
                    const std::string&,
                    const std::string&,
                    const base::string16&) override {}

  void SetPasswordList(
      const std::vector<std::unique_ptr<autofill::PasswordForm>>&) override {}

  void SetPasswordExceptionList(
      const std::vector<std::unique_ptr<autofill::PasswordForm>>&) override {}

#if !defined(OS_ANDROID)
  gfx::NativeWindow GetNativeWindow() const override { return nullptr; }
#endif

 private:
  std::unique_ptr<TestingProfile> profile_;
  PasswordManagerPresenter password_manager_presenter_;

  DISALLOW_COPY_AND_ASSIGN(MockPasswordUIView);
};
}  // namespace

class PasswordManagerPorterTest : public testing::Test {
 protected:
  PasswordManagerPorterTest() {}

  void SetUp() override {
    password_ui_view_.reset(new MockPasswordUIView());
    password_manager_presenter_.reset(
        new MockPasswordManagerPresenter(password_ui_view_.get()));
    password_manager_porter_.reset(
        new MockPasswordManagerPorter(password_manager_presenter_.get()));
    Profile* profile = password_ui_view_->GetProfile();
    web_contents_.reset(MockWebContents::Create(profile, nullptr));
    ui::SelectFileDialog::SetFactory(new TestSelectFileDialogFactory());
  }

  void TearDown() override {
    web_contents_ = nullptr;
    password_manager_porter_ = nullptr;
    password_manager_presenter_ = nullptr;
    password_ui_view_ = nullptr;
  }

  void ImportPasswords() {
    // We can only import passwords after having initially exported them.
    EXPECT_TRUE(password_manager_porter_->passwordsExported);
    password_manager_porter_->PresentFileSelector(
        web_contents_.get(), PasswordManagerPorter::Type::PASSWORD_IMPORT);
    ASSERT_TRUE(password_manager_porter_->passwordsImported);
  }

  void ExportPasswords() {
    password_manager_porter_->PresentFileSelector(
        web_contents_.get(), PasswordManagerPorter::Type::PASSWORD_EXPORT);
    ASSERT_TRUE(password_manager_porter_->passwordsExported);
  }

  std::unique_ptr<MockPasswordUIView> password_ui_view_;
  std::unique_ptr<MockPasswordManagerPresenter> password_manager_presenter_;
  std::unique_ptr<MockPasswordManagerPorter> password_manager_porter_;
  std::unique_ptr<content::WebContents> web_contents_;
  scoped_refptr<password_manager::PasswordStore> store_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerPorterTest);
};

TEST_F(PasswordManagerPorterTest, PasswordExportThenImport) {
  ExportPasswords();
  ImportPasswords();
}
