// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/storage_area.h"

#include "base/command_line.h"
#include "components/crx_file/id_util.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/switches.h"
#include "extensions/renderer/bindings/api_binding_test_util.h"
#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/native_extension_bindings_system.h"
#include "extensions/renderer/native_extension_bindings_system_test_base.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

class CustomTypesTest : public NativeExtensionBindingsSystemUnittest {
 public:
  CustomTypesTest() = default;
  ~CustomTypesTest() override = default;

  void SetUp() override {
    extension_id_ = crx_file::id_util::GenerateId("id");
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kWhitelistedExtensionID, extension_id_);
    NativeExtensionBindingsSystemUnittest::SetUp();
  }

  void RunAccessTest(const char* permission,
                     const char* api_script,
                     const char* use_api_script) {
    scoped_refptr<Extension> extension = ExtensionBuilder("foo")
                                             .AddPermission(permission)
                                             .SetID(extension_id_)
                                             .Build();
    RegisterExtension(extension);

    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Context> context = MainContext();

    ScriptContext* script_context = CreateScriptContext(
        context, extension.get(), Feature::BLESSED_EXTENSION_CONTEXT);
    script_context->set_url(extension->url());

    bindings_system()->UpdateBindingsForContext(script_context);

    v8::Local<v8::Value> api_object =
        V8ValueFromScriptSource(context, api_script);
    ASSERT_TRUE(api_object->IsObject());

    v8::Local<v8::Function> use_api =
        FunctionFromString(context, use_api_script);
    v8::Local<v8::Value> args[] = {api_object};
    RunFunction(use_api, context, arraysize(args), args);

    DisposeContext(context);

    EXPECT_FALSE(binding::IsContextValid(context));
    RunFunctionAndExpectError(use_api, context, arraysize(args), args,
                              "Uncaught Error: Extension context invalidated.");
  }

 private:
  std::string extension_id_;

  DISALLOW_COPY_AND_ASSIGN(CustomTypesTest);
};

TEST_F(CustomTypesTest, ContentSettingsUseAfterInvalidation) {
  RunAccessTest("contentSettings", "chrome.contentSettings.javascript",
                R"((function(setting) {
                     setting.set({
                       primaryPattern: '<all_urls>',
                       setting: 'block'
                     });
                   });)");
}

TEST_F(CustomTypesTest, ChromeSettingsAPIUseAfterInvalidation) {
  RunAccessTest("privacy", "chrome.privacy.websites.doNotTrackEnabled",
                R"((function(setting) { setting.set({value: true}); }))");
}

TEST_F(CustomTypesTest, ChromeSettingsEventUseAfterInvalidation) {
  RunAccessTest("privacy", "chrome.privacy.websites.doNotTrackEnabled",
                R"((function(setting) {
                     setting.onChange.addListener(function() {});
                   });)");
}

TEST_F(CustomTypesTest, EasyUnlockProximityRequiredUseAfterInvalidation) {
  RunAccessTest("preferencesPrivate",
                "chrome.preferencesPrivate.easyUnlockProximityRequired",
                R"((function(setting) {
                     setting.onChange.addListener(function() {});
                   });)");
}

}  // namespace extensions
