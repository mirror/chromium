diff --git a/chrome/browser/spellchecker/spell_check_host_chrome_impl.cc b/chrome/browser/spellchecker/spell_check_host_chrome_impl.cc
index 0aafa6a..aa2a807 100644
--- a/chrome/browser/spellchecker/spell_check_host_chrome_impl.cc
+++ b/chrome/browser/spellchecker/spell_check_host_chrome_impl.cc
@@ -27,7 +27,7 @@ void SpellCheckHostChromeImpl::Create(
     spellcheck::mojom::SpellCheckHostRequest request,
     const service_manager::BindSourceInfo& source_info) {
   mojo::MakeStrongBinding(
-      base::MakeUnique<SpellCheckHostChromeImpl>(source_info.identity),
+      std::make_unique<SpellCheckHostChromeImpl>(source_info.identity),
       std::move(request));
 }
 
diff --git a/chrome/browser/spellchecker/spell_check_host_chrome_impl_unittest.cc b/chrome/browser/spellchecker/spell_check_host_chrome_impl_unittest.cc
index e991fdd..7e978c22 100644
--- a/chrome/browser/spellchecker/spell_check_host_chrome_impl_unittest.cc
+++ b/chrome/browser/spellchecker/spell_check_host_chrome_impl_unittest.cc
@@ -22,7 +22,7 @@
 class TestSpellCheckHostChromeImpl {
  public:
   TestSpellCheckHostChromeImpl()
-      : spellcheck_(base::MakeUnique<SpellcheckService>(&testing_profile_)) {}
+      : spellcheck_(std::make_unique<SpellcheckService>(&testing_profile_)) {}
 
   SpellcheckCustomDictionary& GetCustomDictionary() const {
     EXPECT_NE(nullptr, spellcheck_.get());
diff --git a/chrome/browser/spellchecker/spell_check_panel_host_impl.cc b/chrome/browser/spellchecker/spell_check_panel_host_impl.cc
index 591b905..558e744 100644
--- a/chrome/browser/spellchecker/spell_check_panel_host_impl.cc
+++ b/chrome/browser/spellchecker/spell_check_panel_host_impl.cc
@@ -17,7 +17,7 @@ SpellCheckPanelHostImpl::~SpellCheckPanelHostImpl() = default;
 // static
 void SpellCheckPanelHostImpl::Create(
     spellcheck::mojom::SpellCheckPanelHostRequest request) {
-  mojo::MakeStrongBinding(base::MakeUnique<SpellCheckPanelHostImpl>(),
+  mojo::MakeStrongBinding(std::make_unique<SpellCheckPanelHostImpl>(),
                           std::move(request));
 }
 
diff --git a/chrome/browser/spellchecker/spellcheck_custom_dictionary_unittest.cc b/chrome/browser/spellchecker/spellcheck_custom_dictionary_unittest.cc
index c488d18..e8fab35 100644
--- a/chrome/browser/spellchecker/spellcheck_custom_dictionary_unittest.cc
+++ b/chrome/browser/spellchecker/spellcheck_custom_dictionary_unittest.cc
@@ -58,7 +58,7 @@ syncer::SyncDataList GetAllSyncDataNoLimit(
 
 static std::unique_ptr<KeyedService> BuildSpellcheckService(
     content::BrowserContext* profile) {
-  return base::MakeUnique<SpellcheckService>(static_cast<Profile*>(profile));
+  return std::make_unique<SpellcheckService>(static_cast<Profile*>(profile));
 }
 
 class SpellcheckCustomDictionaryTest : public testing::Test {
@@ -902,7 +902,7 @@ TEST_F(SpellcheckCustomDictionaryTest, LoadDuplicatesAfterSync) {
   EXPECT_TRUE(custom_dictionary->IsSyncing());
 
   OnLoaded(*custom_dictionary,
-           base::MakeUnique<std::set<std::string>>(change.to_add()));
+           std::make_unique<std::set<std::string>>(change.to_add()));
   EXPECT_EQ(0, error_counter);
   EXPECT_TRUE(custom_dictionary->IsSyncing());
 
diff --git a/chrome/browser/spellchecker/spellcheck_factory.cc b/chrome/browser/spellchecker/spellcheck_factory.cc
index f02ff11..20054d1 100644
--- a/chrome/browser/spellchecker/spellcheck_factory.cc
+++ b/chrome/browser/spellchecker/spellcheck_factory.cc
@@ -69,9 +69,9 @@ KeyedService* SpellcheckServiceFactory::BuildServiceInstanceFor(
 void SpellcheckServiceFactory::RegisterProfilePrefs(
     user_prefs::PrefRegistrySyncable* user_prefs) {
   user_prefs->RegisterListPref(spellcheck::prefs::kSpellCheckDictionaries,
-                               base::MakeUnique<base::ListValue>());
+                               std::make_unique<base::ListValue>());
   user_prefs->RegisterListPref(spellcheck::prefs::kSpellCheckForcedDictionaries,
-                               base::MakeUnique<base::ListValue>());
+                               std::make_unique<base::ListValue>());
   // Continue registering kSpellCheckDictionary for preference migration.
   // TODO(estade): remove: crbug.com/751275
   user_prefs->RegisterStringPref(
diff --git a/chrome/browser/spellchecker/spellcheck_language_policy_handler.cc b/chrome/browser/spellchecker/spellcheck_language_policy_handler.cc
index 5e7d5d2..6a436ff 100644
--- a/chrome/browser/spellchecker/spellcheck_language_policy_handler.cc
+++ b/chrome/browser/spellchecker/spellcheck_language_policy_handler.cc
@@ -47,7 +47,7 @@ void SpellcheckLanguagePolicyHandler::ApplyPolicySettings(
   const base::Value::ListStorage& languages = value->GetList();
 
   std::unique_ptr<base::ListValue> forced_language_list =
-      base::MakeUnique<base::ListValue>();
+      std::make_unique<base::ListValue>();
   for (const base::Value& language : languages) {
     std::string current_language =
         spellcheck::GetCorrespondingSpellCheckLanguage(
@@ -61,7 +61,7 @@ void SpellcheckLanguagePolicyHandler::ApplyPolicySettings(
   }
 
   prefs->SetValue(spellcheck::prefs::kSpellCheckEnable,
-                  base::MakeUnique<base::Value>(true));
+                  std::make_unique<base::Value>(true));
   prefs->SetValue(spellcheck::prefs::kSpellCheckForcedDictionaries,
                   std::move(forced_language_list));
 }
diff --git a/chrome/browser/spellchecker/spellcheck_service.cc b/chrome/browser/spellchecker/spellcheck_service.cc
index 8c20211..d83762a 100644
--- a/chrome/browser/spellchecker/spellcheck_service.cc
+++ b/chrome/browser/spellchecker/spellcheck_service.cc
@@ -240,7 +240,7 @@ void SpellcheckService::LoadHunspellDictionaries() {
 
   for (const auto& dictionary : dictionaries) {
     hunspell_dictionaries_.push_back(
-        base::MakeUnique<SpellcheckHunspellDictionary>(
+        std::make_unique<SpellcheckHunspellDictionary>(
             dictionary,
             content::BrowserContext::GetDefaultStoragePartition(context_)
                 ->GetURLRequestContext(),
