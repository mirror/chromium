// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/extension_permission_set.h"

#include "base/logging.h"
#include "base/path_service.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "content/common/json_value_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

static scoped_refptr<Extension> LoadManifest(const std::string& dir,
                                             const std::string& test_file,
                                             int extra_flags) {
  FilePath path;
  PathService::Get(chrome::DIR_TEST_DATA, &path);
  path = path.AppendASCII("extensions")
             .AppendASCII(dir)
             .AppendASCII(test_file);

  JSONFileValueSerializer serializer(path);
  std::string error;
  scoped_ptr<Value> result(serializer.Deserialize(NULL, &error));
  if (!result.get()) {
    EXPECT_EQ("", error);
    return NULL;
  }

  scoped_refptr<Extension> extension = Extension::Create(
      path.DirName(), Extension::INVALID,
      *static_cast<DictionaryValue*>(result.get()),
      Extension::STRICT_ERROR_CHECKS | extra_flags, &error);
  EXPECT_TRUE(extension) << error;
  return extension;
}

static scoped_refptr<Extension> LoadManifest(const std::string& dir,
                                             const std::string& test_file) {
  return LoadManifest(dir, test_file, Extension::NO_FLAGS);
}

void CompareLists(const std::vector<std::string>& expected,
                  const std::vector<std::string>& actual) {
  ASSERT_EQ(expected.size(), actual.size());

  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}

static void AddPattern(URLPatternSet* extent, const std::string& pattern) {
  int schemes = URLPattern::SCHEME_ALL;
  extent->AddPattern(URLPattern(schemes, pattern));
}

static void AssertEqualExtents(const URLPatternSet& extent1,
                               const URLPatternSet& extent2) {
  URLPatternList patterns1 = extent1.patterns();
  URLPatternList patterns2 = extent2.patterns();
  std::set<std::string> strings1;
  EXPECT_EQ(patterns1.size(), patterns2.size());

  for (size_t i = 0; i < patterns1.size(); ++i)
    strings1.insert(patterns1.at(i).GetAsString());

  std::set<std::string> strings2;
  for (size_t i = 0; i < patterns2.size(); ++i)
    strings2.insert(patterns2.at(i).GetAsString());

  EXPECT_EQ(strings1, strings2);
}

} // namespace

class ExtensionAPIPermissionTest : public testing::Test {
};

class ExtensionPermissionSetTest : public testing::Test {
};


// Tests GetByID.
TEST(ExtensionPermissionsInfoTest, GetByID) {
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  ExtensionAPIPermissionSet ids = info->GetAll();
  for (ExtensionAPIPermissionSet::iterator i = ids.begin();
       i != ids.end(); ++i) {
    EXPECT_EQ(*i, info->GetByID(*i)->id());
  }
}

// Tests that GetByName works with normal permission names and aliases.
TEST(ExtensionPermissionsInfoTest, GetByName) {
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  EXPECT_EQ(ExtensionAPIPermission::kTab, info->GetByName("tabs")->id());
  EXPECT_EQ(ExtensionAPIPermission::kManagement,
            info->GetByName("management")->id());
  EXPECT_FALSE(info->GetByName("alsdkfjasldkfj"));
}

TEST(ExtensionPermissionsInfoTest, GetAll) {
  size_t count = 0;
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  ExtensionAPIPermissionSet apis = info->GetAll();
  for (ExtensionAPIPermissionSet::iterator api = apis.begin();
       api != apis.end(); ++api) {
    // Make sure only the valid permission IDs get returned.
    EXPECT_NE(ExtensionAPIPermission::kInvalid, *api);
    EXPECT_NE(ExtensionAPIPermission::kUnknown, *api);
    count++;
  }
  EXPECT_EQ(count, info->get_permission_count());
}

TEST(ExtensionPermissionInfoTest, GetAllByName) {
  std::set<std::string> names;
  names.insert("background");
  names.insert("management");

  // This is an alias of kTab
  names.insert("windows");

  // This unknown name should get dropped.
  names.insert("sdlkfjasdlkfj");

  ExtensionAPIPermissionSet expected;
  expected.insert(ExtensionAPIPermission::kBackground);
  expected.insert(ExtensionAPIPermission::kManagement);
  expected.insert(ExtensionAPIPermission::kTab);

  EXPECT_EQ(expected,
            ExtensionPermissionsInfo::GetInstance()->GetAllByName(names));
}

// Tests that the aliases are properly mapped.
TEST(ExtensionAPIPermissionTest, Aliases) {
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  // tabs: tabs, windows
  std::string tabs_name = "tabs";
  EXPECT_EQ(tabs_name, info->GetByID(ExtensionAPIPermission::kTab)->name());
  EXPECT_EQ(ExtensionAPIPermission::kTab, info->GetByName("tabs")->id());
  EXPECT_EQ(ExtensionAPIPermission::kTab, info->GetByName("windows")->id());

  // unlimitedStorage: unlimitedStorage, unlimited_storage
  std::string storage_name = "unlimitedStorage";
  EXPECT_EQ(storage_name, info->GetByID(
      ExtensionAPIPermission::kUnlimitedStorage)->name());
  EXPECT_EQ(ExtensionAPIPermission::kUnlimitedStorage,
            info->GetByName("unlimitedStorage")->id());
  EXPECT_EQ(ExtensionAPIPermission::kUnlimitedStorage,
            info->GetByName("unlimited_storage")->id());
}

TEST(ExtensionAPIPermissionTest, HostedAppPermissions) {
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  ExtensionAPIPermissionSet hosted_perms;
  hosted_perms.insert(ExtensionAPIPermission::kBackground);
  hosted_perms.insert(ExtensionAPIPermission::kClipboardRead);
  hosted_perms.insert(ExtensionAPIPermission::kClipboardWrite);
  hosted_perms.insert(ExtensionAPIPermission::kChromePrivate);
  hosted_perms.insert(ExtensionAPIPermission::kExperimental);
  hosted_perms.insert(ExtensionAPIPermission::kGeolocation);
  hosted_perms.insert(ExtensionAPIPermission::kNotification);
  hosted_perms.insert(ExtensionAPIPermission::kUnlimitedStorage);
  hosted_perms.insert(ExtensionAPIPermission::kWebstorePrivate);

  ExtensionAPIPermissionSet perms = info->GetAll();
  size_t count = 0;
  for (ExtensionAPIPermissionSet::iterator i = perms.begin();
       i != perms.end(); ++i) {
    count += hosted_perms.count(*i);
    EXPECT_EQ(hosted_perms.count(*i) > 0, info->GetByID(*i)->is_hosted_app());
  }

  EXPECT_EQ(9u, count);
  EXPECT_EQ(9u, info->get_hosted_app_permission_count());
}

TEST(ExtensionAPIPermissionTest, ComponentOnlyPermissions) {
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  ExtensionAPIPermissionSet private_perms;
  private_perms.insert(ExtensionAPIPermission::kChromeosInfoPrivate);
  private_perms.insert(ExtensionAPIPermission::kFileBrowserPrivate);
  private_perms.insert(ExtensionAPIPermission::kMediaPlayerPrivate);
  private_perms.insert(ExtensionAPIPermission::kWebstorePrivate);

  ExtensionAPIPermissionSet perms = info->GetAll();
  int count = 0;
  for (ExtensionAPIPermissionSet::iterator i = perms.begin();
       i != perms.end(); ++i) {
    count += private_perms.count(*i);
    EXPECT_EQ(private_perms.count(*i) > 0,
              info->GetByID(*i)->is_component_only());
  }

  EXPECT_EQ(4, count);
}

TEST(ExtensionPermissionSetTest, EffectiveHostPermissions) {
  scoped_refptr<Extension> extension;
  const ExtensionPermissionSet* permissions = NULL;

  extension = LoadManifest("effective_host_permissions", "empty.json");
  permissions = extension->permission_set();
  EXPECT_EQ(0u, extension->GetEffectiveHostPermissions().patterns().size());
  EXPECT_FALSE(permissions->HasEffectiveAccessToURL(
      GURL("http://www.google.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions", "one_host.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://www.google.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToURL(
      GURL("https://www.google.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions",
                           "one_host_wildcard.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("http://google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://foo.google.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions", "two_hosts.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://www.google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://www.reddit.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions",
                           "https_not_considered.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("http://google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("https://google.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions",
                           "two_content_scripts.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("http://google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://www.reddit.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(
      GURL("http://news.ycombinator.com")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions", "all_hosts.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("http://test/")));
  EXPECT_FALSE(permissions->HasEffectiveAccessToURL(GURL("https://test/")));
  EXPECT_TRUE(
      permissions->HasEffectiveAccessToURL(GURL("http://www.google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions", "all_hosts2.json");
  permissions = extension->permission_set();
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("http://test/")));
  EXPECT_TRUE(
      permissions->HasEffectiveAccessToURL(GURL("http://www.google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToAllHosts());

  extension = LoadManifest("effective_host_permissions", "all_hosts3.json");
  permissions = extension->permission_set();
  EXPECT_FALSE(permissions->HasEffectiveAccessToURL(GURL("http://test/")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToURL(GURL("https://test/")));
  EXPECT_TRUE(
      permissions->HasEffectiveAccessToURL(GURL("http://www.google.com")));
  EXPECT_TRUE(permissions->HasEffectiveAccessToAllHosts());
}

TEST(ExtensionPermissionSetTest, ExplicitAccessToOrigin) {
  ExtensionAPIPermissionSet apis;
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;

  AddPattern(&explicit_hosts, "http://*.google.com/*");
  // The explicit host paths should get set to /*.
  AddPattern(&explicit_hosts, "http://www.example.com/a/particular/path/*");

  ExtensionPermissionSet perm_set(apis, explicit_hosts, scriptable_hosts);
  ASSERT_TRUE(perm_set.HasExplicitAccessToOrigin(
      GURL("http://www.google.com/")));
  ASSERT_TRUE(perm_set.HasExplicitAccessToOrigin(
      GURL("http://test.google.com/")));
  ASSERT_TRUE(perm_set.HasExplicitAccessToOrigin(
      GURL("http://www.example.com")));
  ASSERT_TRUE(perm_set.HasEffectiveAccessToURL(
      GURL("http://www.example.com")));
  ASSERT_FALSE(perm_set.HasExplicitAccessToOrigin(
      GURL("http://test.example.com")));
}

TEST(ExtensionPermissionSetTest, CreateUnion) {
  ExtensionAPIPermissionSet apis1;
  ExtensionAPIPermissionSet apis2;
  ExtensionAPIPermissionSet expected_apis;

  URLPatternSet explicit_hosts1;
  URLPatternSet explicit_hosts2;
  URLPatternSet expected_explicit_hosts;

  URLPatternSet scriptable_hosts1;
  URLPatternSet scriptable_hosts2;
  URLPatternSet expected_scriptable_hosts;

  URLPatternSet effective_hosts;

  scoped_ptr<ExtensionPermissionSet> set1;
  scoped_ptr<ExtensionPermissionSet> set2;
  scoped_ptr<ExtensionPermissionSet> union_set;

  // Union with an empty set.
  apis1.insert(ExtensionAPIPermission::kTab);
  apis1.insert(ExtensionAPIPermission::kBackground);
  expected_apis.insert(ExtensionAPIPermission::kTab);
  expected_apis.insert(ExtensionAPIPermission::kBackground);

  AddPattern(&explicit_hosts1, "http://*.google.com/*");
  AddPattern(&expected_explicit_hosts, "http://*.google.com/*");
  AddPattern(&effective_hosts, "http://*.google.com/*");

  set1.reset(new ExtensionPermissionSet(
      apis1, explicit_hosts1, scriptable_hosts1));
  set2.reset(new ExtensionPermissionSet(
      apis2, explicit_hosts2, scriptable_hosts2));
  union_set.reset(ExtensionPermissionSet::CreateUnion(set1.get(), set2.get()));

  EXPECT_FALSE(union_set->HasEffectiveFullAccess());
  EXPECT_EQ(expected_apis, union_set->apis());
  AssertEqualExtents(expected_explicit_hosts, union_set->explicit_hosts());
  AssertEqualExtents(expected_scriptable_hosts, union_set->scriptable_hosts());
  AssertEqualExtents(expected_explicit_hosts, union_set->effective_hosts());

  // Now use a real second set.
  apis2.insert(ExtensionAPIPermission::kTab);
  apis2.insert(ExtensionAPIPermission::kProxy);
  apis2.insert(ExtensionAPIPermission::kClipboardWrite);
  apis2.insert(ExtensionAPIPermission::kPlugin);
  expected_apis.insert(ExtensionAPIPermission::kTab);
  expected_apis.insert(ExtensionAPIPermission::kProxy);
  expected_apis.insert(ExtensionAPIPermission::kClipboardWrite);
  expected_apis.insert(ExtensionAPIPermission::kPlugin);

  AddPattern(&explicit_hosts2, "http://*.example.com/*");
  AddPattern(&scriptable_hosts2, "http://*.google.com/*");
  AddPattern(&expected_explicit_hosts, "http://*.example.com/*");
  AddPattern(&expected_scriptable_hosts, "http://*.google.com/*");

  effective_hosts.ClearPatterns();
  AddPattern(&effective_hosts, "<all_urls>");

  set2.reset(new ExtensionPermissionSet(
      apis2, explicit_hosts2, scriptable_hosts2));
  union_set.reset(ExtensionPermissionSet::CreateUnion(set1.get(), set2.get()));
  EXPECT_TRUE(union_set->HasEffectiveFullAccess());
  EXPECT_TRUE(union_set->HasEffectiveAccessToAllHosts());
  EXPECT_EQ(expected_apis, union_set->apis());
  AssertEqualExtents(expected_explicit_hosts, union_set->explicit_hosts());
  AssertEqualExtents(expected_scriptable_hosts, union_set->scriptable_hosts());
  AssertEqualExtents(effective_hosts, union_set->effective_hosts());
}

TEST(ExtensionPermissionSetTest, HasLessPrivilegesThan) {
  const struct {
    const char* base_name;
    // Increase these sizes if you have more than 10.
    const char* granted_apis[10];
    const char* granted_hosts[10];
    bool full_access;
    bool expect_increase;
  } kTests[] = {
    { "allhosts1", {NULL}, {"http://*/", NULL}, false,
      false },  // all -> all
    { "allhosts2", {NULL}, {"http://*/", NULL}, false,
      false },  // all -> one
    { "allhosts3", {NULL}, {NULL}, false, true },  // one -> all
    { "hosts1", {NULL},
      {"http://www.google.com/", "http://www.reddit.com/", NULL}, false,
      false },  // http://a,http://b -> http://a,http://b
    { "hosts2", {NULL},
      {"http://www.google.com/", "http://www.reddit.com/", NULL}, false,
      true },  // http://a,http://b -> https://a,http://*.b
    { "hosts3", {NULL},
      {"http://www.google.com/", "http://www.reddit.com/", NULL}, false,
      false },  // http://a,http://b -> http://a
    { "hosts4", {NULL},
      {"http://www.google.com/", NULL}, false,
      true },  // http://a -> http://a,http://b
    { "hosts5", {"tabs", "notifications", NULL},
      {"http://*.example.com/", "http://*.example.com/*",
       "http://*.example.co.uk/*", "http://*.example.com.au/*",
       NULL}, false,
      false },  // http://a,b,c -> http://a,b,c + https://a,b,c
    { "hosts6", {"tabs", "notifications", NULL},
      {"http://*.example.com/", "http://*.example.com/*", NULL}, false,
      false },  // http://a.com -> http://a.com + http://a.co.uk
    { "permissions1", {"tabs", NULL},
      {NULL}, false, false },  // tabs -> tabs
    { "permissions2", {"tabs", NULL},
      {NULL}, false, true },  // tabs -> tabs,bookmarks
    { "permissions3", {NULL},
      {"http://*/*", NULL},
      false, true },  // http://a -> http://a,tabs
    { "permissions5", {"bookmarks", NULL},
      {NULL}, false, true },  // bookmarks -> bookmarks,history
#if !defined(OS_CHROMEOS)  // plugins aren't allowed in ChromeOS
    { "permissions4", {NULL},
      {NULL}, true, false },  // plugin -> plugin,tabs
    { "plugin1", {NULL},
      {NULL}, true, false },  // plugin -> plugin
    { "plugin2", {NULL},
      {NULL}, true, false },  // plugin -> none
    { "plugin3", {NULL},
      {NULL}, false, true },  // none -> plugin
#endif
    { "storage", {NULL},
      {NULL}, false, false },  // none -> storage
    { "notifications", {NULL},
      {NULL}, false, false }  // none -> notifications
  };

  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTests); ++i) {
    scoped_refptr<Extension> old_extension(
        LoadManifest("allow_silent_upgrade",
                     std::string(kTests[i].base_name) + "_old.json"));
    scoped_refptr<Extension> new_extension(
        LoadManifest("allow_silent_upgrade",
                     std::string(kTests[i].base_name) + "_new.json"));

    ExtensionAPIPermissionSet granted_apis;
    for (size_t j = 0; kTests[i].granted_apis[j] != NULL; ++j) {
      granted_apis.insert(info->GetByName(kTests[i].granted_apis[j])->id());
    }

    URLPatternSet granted_hosts;
    for (size_t j = 0; kTests[i].granted_hosts[j] != NULL; ++j)
      AddPattern(&granted_hosts, kTests[i].granted_hosts[j]);

    EXPECT_TRUE(new_extension.get()) << kTests[i].base_name << "_new.json";
    if (!new_extension.get())
      continue;

    const ExtensionPermissionSet* old_p = old_extension->permission_set();
    const ExtensionPermissionSet* new_p = new_extension->permission_set();

    EXPECT_EQ(kTests[i].expect_increase, old_p->HasLessPrivilegesThan(new_p))
        << kTests[i].base_name;
  }
}

TEST(ExtensionPermissionSetTest, PermissionMessages) {
  // Ensure that all permissions that needs to show install UI actually have
  // strings associated with them.
  ExtensionAPIPermissionSet skip;

  skip.insert(ExtensionAPIPermission::kDefault);

  // These are considered "nuisance" or "trivial" permissions that don't need
  // a prompt.
  skip.insert(ExtensionAPIPermission::kContextMenus);
  skip.insert(ExtensionAPIPermission::kIdle);
  skip.insert(ExtensionAPIPermission::kNotification);
  skip.insert(ExtensionAPIPermission::kUnlimitedStorage);
  skip.insert(ExtensionAPIPermission::kContentSettings);

  // TODO(erikkay) add a string for this permission.
  skip.insert(ExtensionAPIPermission::kBackground);

  skip.insert(ExtensionAPIPermission::kClipboardWrite);

  // The cookie permission does nothing unless you have associated host
  // permissions.
  skip.insert(ExtensionAPIPermission::kCookie);

  // The proxy permission is warned as part of host permission checks.
  skip.insert(ExtensionAPIPermission::kProxy);

  // This permission requires explicit user action (context menu handler)
  // so we won't prompt for it for now.
  skip.insert(ExtensionAPIPermission::kFileBrowserHandler);

  // If you've turned on the experimental command-line flag, we don't need
  // to warn you further.
  skip.insert(ExtensionAPIPermission::kExperimental);

  // These are private.
  skip.insert(ExtensionAPIPermission::kWebstorePrivate);
  skip.insert(ExtensionAPIPermission::kFileBrowserPrivate);
  skip.insert(ExtensionAPIPermission::kMediaPlayerPrivate);
  skip.insert(ExtensionAPIPermission::kChromePrivate);
  skip.insert(ExtensionAPIPermission::kChromeosInfoPrivate);
  skip.insert(ExtensionAPIPermission::kWebSocketProxyPrivate);

  // Warned as part of host permissions.
  skip.insert(ExtensionAPIPermission::kDevtools);
  ExtensionPermissionsInfo* info = ExtensionPermissionsInfo::GetInstance();
  ExtensionAPIPermissionSet permissions = info->GetAll();
  for (ExtensionAPIPermissionSet::const_iterator i = permissions.begin();
       i != permissions.end(); ++i) {
    ExtensionAPIPermission* permission = info->GetByID(*i);
    EXPECT_TRUE(permission);
    if (skip.count(*i)) {
      EXPECT_EQ(ExtensionPermissionMessage::kNone, permission->message_id())
          << "unexpected message_id for " << permission->name();
    } else {
      EXPECT_NE(ExtensionPermissionMessage::kNone, permission->message_id())
          << "missing message_id for " << permission->name();
    }
  }
}

// Tests the default permissions (empty API permission set).
TEST(ExtensionPermissionSetTest, DefaultFunctionAccess) {
  const struct {
    const char* permission_name;
    bool expect_success;
  } kTests[] = {
    // Negative test.
    { "non_existing_permission", false },
    // Test default module/package permission.
    { "browserAction",  true },
    { "browserActions", true },
    { "devtools",       true },
    { "extension",      true },
    { "i18n",           true },
    { "pageAction",     true },
    { "pageActions",    true },
    { "test",           true },
    // Some negative tests.
    { "bookmarks",      false },
    { "cookies",        false },
    { "history",        false },
    { "tabs.onUpdated", false },
    // Make sure we find the module name after stripping '.' and '/'.
    { "browserAction/abcd/onClick",  true },
    { "browserAction.abcd.onClick",  true },
    // Test Tabs functions.
    { "tabs.create",      true},
    { "tabs.update",      true},
    { "tabs.getSelected", false},
  };

  ExtensionPermissionSet permissions;
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTests); ++i) {
    EXPECT_EQ(kTests[i].expect_success,
              permissions.HasAccessToFunction(kTests[i].permission_name));
  }
}

TEST(ExtensionPermissionSetTest, GetWarningMessages_ManyHosts) {
  scoped_refptr<Extension> extension;

  extension = LoadManifest("permissions", "many-hosts.json");
  std::vector<string16> warnings =
      extension->permission_set()->GetWarningMessages();
  ASSERT_EQ(1u, warnings.size());
  EXPECT_EQ("Your data on www.google.com and encrypted.google.com",
            UTF16ToUTF8(warnings[0]));
}

TEST(ExtensionPermissionSetTest, GetWarningMessages_Plugins) {
  scoped_refptr<Extension> extension;
  scoped_ptr<ExtensionPermissionSet> permissions;

  extension = LoadManifest("permissions", "plugins.json");
  std::vector<string16> warnings =
      extension->permission_set()->GetWarningMessages();
  // We don't parse the plugins key on Chrome OS, so it should not ask for any
  // permissions.
#if defined(OS_CHROMEOS)
  ASSERT_EQ(0u, warnings.size());
#else
  ASSERT_EQ(1u, warnings.size());
  EXPECT_EQ("All data on your computer and the websites you visit",
            UTF16ToUTF8(warnings[0]));
#endif
}

TEST(ExtensionPermissionSetTest, GetDistinctHostsForDisplay) {
  scoped_ptr<ExtensionPermissionSet> perm_set;
  ExtensionAPIPermissionSet empty_perms;
  std::vector<std::string> expected;
  expected.push_back("www.foo.com");
  expected.push_back("www.bar.com");
  expected.push_back("www.baz.com");
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;

  {
    SCOPED_TRACE("no dupes");

    // Simple list with no dupes.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.com/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.bar.com/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.baz.com/path"));
    perm_set.reset(new ExtensionPermissionSet(
       empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("two dupes");

    // Add some dupes.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.com/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.baz.com/path"));
    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("schemes differ");

    // Add a pattern that differs only by scheme. This should be filtered out.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTPS, "https://www.bar.com/path"));
    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("paths differ");

    // Add some dupes by path.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.bar.com/pathypath"));
    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("subdomains differ");

    // We don't do anything special for subdomains.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://monkey.www.bar.com/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://bar.com/path"));

    expected.push_back("monkey.www.bar.com");
    expected.push_back("bar.com");

    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("RCDs differ");

    // Now test for RCD uniquing.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.com/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.co.uk/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.de/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.ca.us/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.net/path"));
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.com.my/path"));

    // This is an unknown RCD, which shouldn't be uniqued out.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.xyzzy/path"));
    // But it should only occur once.
    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.xyzzy/path"));

    expected.push_back("www.foo.xyzzy");

    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("wildcards");

    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://*.google.com/*"));

    expected.push_back("*.google.com");

    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }

  {
    SCOPED_TRACE("scriptable hosts");
    explicit_hosts.ClearPatterns();
    scriptable_hosts.ClearPatterns();
    expected.clear();

    explicit_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://*.google.com/*"));
    scriptable_hosts.AddPattern(
        URLPattern(URLPattern::SCHEME_HTTP, "http://*.example.com/*"));

    expected.push_back("*.google.com");
    expected.push_back("*.example.com");

    perm_set.reset(new ExtensionPermissionSet(
            empty_perms, explicit_hosts, scriptable_hosts));
    CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
  }
}

TEST(ExtensionPermissionSetTest, GetDistinctHostsForDisplay_ComIsBestRcd) {
  scoped_ptr<ExtensionPermissionSet> perm_set;
  ExtensionAPIPermissionSet empty_perms;
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.ca/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.org/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.co.uk/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.net/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.jp/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.com/path"));

  std::vector<std::string> expected;
  expected.push_back("www.foo.com");
  perm_set.reset(new ExtensionPermissionSet(
      empty_perms, explicit_hosts, scriptable_hosts));
  CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
}

TEST(ExtensionPermissionSetTest, GetDistinctHostsForDisplay_NetIs2ndBestRcd) {
  scoped_ptr<ExtensionPermissionSet> perm_set;
  ExtensionAPIPermissionSet empty_perms;
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.ca/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.org/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.co.uk/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.net/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.jp/path"));
  // No http://www.foo.com/path

  std::vector<std::string> expected;
  expected.push_back("www.foo.net");
  perm_set.reset(new ExtensionPermissionSet(
      empty_perms, explicit_hosts, scriptable_hosts));
  CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
}

TEST(ExtensionPermissionSetTest,
     GetDistinctHostsForDisplay_OrgIs3rdBestRcd) {
  scoped_ptr<ExtensionPermissionSet> perm_set;
  ExtensionAPIPermissionSet empty_perms;
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.ca/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.org/path"));
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.co.uk/path"));
  // No http://www.foo.net/path
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.jp/path"));
  // No http://www.foo.com/path

  std::vector<std::string> expected;
  expected.push_back("www.foo.org");
  perm_set.reset(new ExtensionPermissionSet(
      empty_perms, explicit_hosts, scriptable_hosts));
  CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
}

TEST(ExtensionPermissionSetTest,
     GetDistinctHostsForDisplay_FirstInListIs4thBestRcd) {
  scoped_ptr<ExtensionPermissionSet> perm_set;
  ExtensionAPIPermissionSet empty_perms;
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.ca/path"));
  // No http://www.foo.org/path
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.co.uk/path"));
  // No http://www.foo.net/path
  explicit_hosts.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.foo.jp/path"));
  // No http://www.foo.com/path

  std::vector<std::string> expected;
  expected.push_back("www.foo.ca");
  perm_set.reset(new ExtensionPermissionSet(
      empty_perms, explicit_hosts, scriptable_hosts));
  CompareLists(expected, perm_set->GetDistinctHostsForDisplay());
}

TEST(ExtensionPermissionSetTest, HasLessHostPrivilegesThan) {
  URLPatternSet elist1;
  URLPatternSet elist2;
  URLPatternSet slist1;
  URLPatternSet slist2;
  scoped_ptr<ExtensionPermissionSet> set1;
  scoped_ptr<ExtensionPermissionSet> set2;
  ExtensionAPIPermissionSet empty_perms;
  elist1.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com.hk/path"));
  elist1.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com/path"));

  // Test that the host order does not matter.
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com/path"));
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com.hk/path"));

  set1.reset(new ExtensionPermissionSet(empty_perms, elist1, slist1));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));

  EXPECT_FALSE(set1->HasLessHostPrivilegesThan(set2.get()));
  EXPECT_FALSE(set2->HasLessHostPrivilegesThan(set1.get()));

  // Test that paths are ignored.
  elist2.ClearPatterns();
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com/*"));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));
  EXPECT_FALSE(set1->HasLessHostPrivilegesThan(set2.get()));
  EXPECT_FALSE(set2->HasLessHostPrivilegesThan(set1.get()));

  // Test that RCDs are ignored.
  elist2.ClearPatterns();
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com.hk/*"));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));
  EXPECT_FALSE(set1->HasLessHostPrivilegesThan(set2.get()));
  EXPECT_FALSE(set2->HasLessHostPrivilegesThan(set1.get()));

  // Test that subdomain wildcards are handled properly.
  elist2.ClearPatterns();
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://*.google.com.hk/*"));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));
  EXPECT_TRUE(set1->HasLessHostPrivilegesThan(set2.get()));
  //TODO(jstritar): Does not match subdomains properly. http://crbug.com/65337
  //EXPECT_FALSE(set2->HasLessHostPrivilegesThan(set1.get()));

  // Test that different domains count as different hosts.
  elist2.ClearPatterns();
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.google.com/path"));
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://www.example.org/path"));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));
  EXPECT_TRUE(set1->HasLessHostPrivilegesThan(set2.get()));
  EXPECT_FALSE(set2->HasLessHostPrivilegesThan(set1.get()));

  // Test that different subdomains count as different hosts.
  elist2.ClearPatterns();
  elist2.AddPattern(
      URLPattern(URLPattern::SCHEME_HTTP, "http://mail.google.com/*"));
  set2.reset(new ExtensionPermissionSet(empty_perms, elist2, slist2));
  EXPECT_TRUE(set1->HasLessHostPrivilegesThan(set2.get()));
  EXPECT_TRUE(set2->HasLessHostPrivilegesThan(set1.get()));
}

TEST(ExtensionPermissionSetTest, GetAPIsAsStrings) {
  ExtensionAPIPermissionSet apis;
  URLPatternSet empty_set;

  apis.insert(ExtensionAPIPermission::kProxy);
  apis.insert(ExtensionAPIPermission::kBackground);
  apis.insert(ExtensionAPIPermission::kNotification);
  apis.insert(ExtensionAPIPermission::kTab);

  ExtensionPermissionSet perm_set(apis, empty_set, empty_set);
  std::set<std::string> api_names = perm_set.GetAPIsAsStrings();

  // The result is correct if it has the same number of elements
  // and we can convert it back to the id set.
  EXPECT_EQ(4u, api_names.size());
  EXPECT_EQ(apis,
            ExtensionPermissionsInfo::GetInstance()->GetAllByName(api_names));
}

TEST(ExtensionPermissionSetTest, IsEmpty) {
  ExtensionAPIPermissionSet empty_apis;
  URLPatternSet empty_extent;

  ExtensionPermissionSet perm_set;
  EXPECT_TRUE(perm_set.IsEmpty());

  perm_set = ExtensionPermissionSet(empty_apis, empty_extent, empty_extent);
  EXPECT_TRUE(perm_set.IsEmpty());

  ExtensionAPIPermissionSet non_empty_apis;
  non_empty_apis.insert(ExtensionAPIPermission::kBackground);
  perm_set = ExtensionPermissionSet(
      non_empty_apis, empty_extent, empty_extent);
  EXPECT_FALSE(perm_set.IsEmpty());

  // Try non standard host
  URLPatternSet non_empty_extent;
  AddPattern(&non_empty_extent, "http://www.google.com/*");

  perm_set = ExtensionPermissionSet(
      empty_apis, non_empty_extent, empty_extent);
  EXPECT_FALSE(perm_set.IsEmpty());

  perm_set = ExtensionPermissionSet(
      empty_apis, empty_extent, non_empty_extent);
  EXPECT_FALSE(perm_set.IsEmpty());
}
