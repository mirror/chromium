// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/pref_service_factory.h"

#include "base/barrier_closure.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/sequenced_worker_pool_owner.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/value_map_pref_store.h"
#include "components/prefs/writeable_pref_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/preferences/pref_store_impl.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"
#include "services/preferences/public/cpp/pref_service_main.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"

namespace prefs {
namespace {

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  ServiceTestClient(
      service_manager::test::ServiceTest* test,
      base::Callback<std::unique_ptr<service_manager::Service>()>
          service_factory,
      base::OnceCallback<void(service_manager::Connector*)> connector_callback)
      : service_manager::test::ServiceTestClient(test),
        service_factory_(std::move(service_factory)),
        connector_callback_(std::move(connector_callback)) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&ServiceTestClient::Create, base::Unretained(this)));
  }

 protected:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    if (name == prefs::mojom::kServiceName) {
      pref_service_context_.reset(new service_manager::ServiceContext(
          service_factory_.Run(), std::move(request)));
    } else if (name == "prefs_unittest_helper") {
      test_helper_service_context_ =
          std::make_unique<service_manager::ServiceContext>(
              std::make_unique<service_manager::Service>(), std::move(request));
      std::move(connector_callback_)
          .Run(test_helper_service_context_->connector());
    }
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  base::Callback<std::unique_ptr<service_manager::Service>()> service_factory_;
  std::unique_ptr<service_manager::ServiceContext> pref_service_context_;
  std::unique_ptr<service_manager::ServiceContext> test_helper_service_context_;
  base::OnceCallback<void(service_manager::Connector*)> connector_callback_;
};

constexpr int kPrefServiceFactoryUnittestInitialValue = 1;
constexpr int kPrefServiceFactoryUnittestUpdatedValue = 2;
constexpr char kPrefServiceFactoryUnittestKey[] = "some_key";
constexpr char kPrefServiceFactoryUnittestOtherKey[] = "some_other_key";
constexpr char kPrefServiceFactoryUnittestDictionaryKey[] = "a.dictionary.pref";
constexpr char kPrefServiceFactoryUnittestInitialKey[] = "initial_key";
constexpr char kPrefServiceFactoryUnittestOtherInitialKey[] =
    "other_initial_key";

class PrefServiceFactoryTest : public service_manager::test::ServiceTest {
 public:
  PrefServiceFactoryTest() : ServiceTest("prefs_unittests") {}

 protected:
  void SetUp() override {
    above_user_prefs_pref_store_ = new ValueMapPrefStore();
    below_user_prefs_pref_store_ = new ValueMapPrefStore();
    auto user_prefs = base::MakeRefCounted<InMemoryPrefStore>();
    PrefServiceFactory factory;
    service_factory_ = std::make_unique<InProcessPrefServiceFactory>();
    auto delegate = service_factory_->CreateDelegate();
    auto pref_registry = GetInitialPrefRegistry();
    delegate->InitPrefRegistry(pref_registry.get());
    factory.set_user_prefs(user_prefs);
    factory.set_recommended_prefs(below_user_prefs_pref_store_);
    factory.set_command_line_prefs(above_user_prefs_pref_store_);
    CustomizePrefDelegateAndFactory(delegate.get(), &factory);
    pref_service_ = factory.Create(pref_registry.get(), std::move(delegate));

    base::RunLoop run_loop;
    connector_callback_ =
        base::BindOnce(&PrefServiceFactoryTest::SetOtherClientConnector,
                       base::Unretained(this), run_loop.QuitClosure());
    ServiceTest::SetUp();
    ASSERT_TRUE(profile_dir_.CreateUniqueTempDir());
    connector()->StartService("prefs_unittest_helper");
    run_loop.Run();
  }

  virtual void CustomizePrefDelegateAndFactory(
      PrefValueStore::Delegate* delegate,
      PrefServiceFactory* factory) {}

  service_manager::Connector* other_client_connector() {
    return other_client_connector_;
  }

  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override {
    return std::make_unique<ServiceTestClient>(
        this, service_factory_->CreatePrefServiceFactory(),
        std::move(connector_callback_));
  }

  // Create a fully initialized PrefService synchronously.
  std::unique_ptr<PrefService> Create() {
    return CreateImpl(CreateDefaultPrefRegistry(), connector());
  }

  std::unique_ptr<PrefService> CreateForeign() {
    return CreateImpl(CreateDefaultForeignPrefRegistry(),
                      other_client_connector());
  }

  std::unique_ptr<PrefService> CreateImpl(
      scoped_refptr<PrefRegistry> pref_registry,
      service_manager::Connector* connector) {
    std::unique_ptr<PrefService> pref_service;
    base::RunLoop run_loop;
    CreateAsync(std::move(pref_registry), connector, run_loop.QuitClosure(),
                &pref_service);
    run_loop.Run();
    return pref_service;
  }

  void CreateAsync(scoped_refptr<PrefRegistry> pref_registry,
                   service_manager::Connector* connector,
                   base::Closure callback,
                   std::unique_ptr<PrefService>* out) {
    ConnectToPrefService(
        connector, std::move(pref_registry),
        base::Bind(&PrefServiceFactoryTest::OnCreate, callback, out));
  }

  scoped_refptr<PrefRegistrySimple> GetInitialPrefRegistry() {
    if (!pref_registry_) {
      pref_registry_ = base::MakeRefCounted<PrefRegistrySimple>();
      pref_registry_->RegisterIntegerPref(
          kPrefServiceFactoryUnittestInitialKey,
          kPrefServiceFactoryUnittestInitialValue, PrefRegistry::PUBLIC);
      pref_registry_->RegisterIntegerPref(
          kPrefServiceFactoryUnittestOtherInitialKey,
          kPrefServiceFactoryUnittestInitialValue, PrefRegistry::PUBLIC);
    }
    return pref_registry_;
  }

  scoped_refptr<PrefRegistrySimple> CreateDefaultPrefRegistry() {
    auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
    pref_registry->RegisterIntegerPref(kPrefServiceFactoryUnittestKey,
                                       kPrefServiceFactoryUnittestInitialValue,
                                       PrefRegistry::PUBLIC);
    pref_registry->RegisterIntegerPref(kPrefServiceFactoryUnittestOtherKey,
                                       kPrefServiceFactoryUnittestInitialValue,
                                       PrefRegistry::PUBLIC);
    pref_registry->RegisterDictionaryPref(
        kPrefServiceFactoryUnittestDictionaryKey, PrefRegistry::PUBLIC);
    pref_registry->RegisterForeignPref(kPrefServiceFactoryUnittestInitialKey);
    pref_registry->RegisterForeignPref(
        kPrefServiceFactoryUnittestOtherInitialKey);
    return pref_registry;
  }

  scoped_refptr<PrefRegistrySimple> CreateDefaultForeignPrefRegistry() {
    auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
    pref_registry->RegisterForeignPref(kPrefServiceFactoryUnittestKey);
    pref_registry->RegisterForeignPref(kPrefServiceFactoryUnittestOtherKey);
    pref_registry->RegisterForeignPref(
        kPrefServiceFactoryUnittestDictionaryKey);
    return pref_registry;
  }

  // Wait until first update of the pref |key| in |pref_service| synchronously.
  void WaitForPrefChange(PrefService* pref_service, const std::string& key) {
    PrefChangeRegistrar registrar;
    registrar.Init(pref_service);
    base::RunLoop run_loop;
    registrar.Add(key, base::Bind(&OnPrefChanged, run_loop.QuitClosure(), key));
    run_loop.Run();
  }

  WriteablePrefStore* above_user_prefs_pref_store() {
    return above_user_prefs_pref_store_.get();
  }
  WriteablePrefStore* below_user_prefs_pref_store() {
    return below_user_prefs_pref_store_.get();
  }
  PrefService* pref_service() { return pref_service_.get(); }

 private:
  void SetOtherClientConnector(base::OnceClosure done,
                               service_manager::Connector* connector) {
    other_client_connector_ = connector;
    std::move(done).Run();
  }

  // Called when the PrefService has been created.
  static void OnCreate(const base::Closure& quit_closure,
                       std::unique_ptr<PrefService>* out,
                       std::unique_ptr<PrefService> pref_service) {
    DCHECK(pref_service);
    *out = std::move(pref_service);
    quit_closure.Run();
  }

  static void OnPrefChanged(const base::Closure& quit_closure,
                            const std::string& expected_path,
                            const std::string& path) {
    if (path == expected_path)
      quit_closure.Run();
  }

  base::ScopedTempDir profile_dir_;
  scoped_refptr<WriteablePrefStore> above_user_prefs_pref_store_;
  scoped_refptr<WriteablePrefStore> below_user_prefs_pref_store_;
  scoped_refptr<PrefRegistrySimple> pref_registry_;
  std::unique_ptr<PrefService> pref_service_;
  service_manager::Connector* other_client_connector_ = nullptr;
  base::OnceCallback<void(service_manager::Connector*)> connector_callback_;
  std::unique_ptr<InProcessPrefServiceFactory> service_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefServiceFactoryTest);
};

// Check that a single client can set and read back values.
TEST_F(PrefServiceFactoryTest, Basic) {
  auto pref_service = Create();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  pref_service->SetInteger(kPrefServiceFactoryUnittestKey,
                           kPrefServiceFactoryUnittestUpdatedValue);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
}

// Check that updates in one client eventually propagates to the other.
TEST_F(PrefServiceFactoryTest, MultipleClients) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestKey));
  pref_service->SetInteger(kPrefServiceFactoryUnittestKey,
                           kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestKey));
}

// Check that updates in one client eventually propagates to the other.
TEST_F(PrefServiceFactoryTest, InternalAndExternalClients) {
  auto pref_service2 = Create();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service()->GetInteger(kPrefServiceFactoryUnittestInitialKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestInitialKey));
  EXPECT_EQ(
      kPrefServiceFactoryUnittestInitialValue,
      pref_service()->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
  EXPECT_EQ(
      kPrefServiceFactoryUnittestInitialValue,
      pref_service2->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
  pref_service()->SetInteger(kPrefServiceFactoryUnittestInitialKey,
                             kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestInitialKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestInitialKey));

  pref_service2->SetInteger(kPrefServiceFactoryUnittestOtherInitialKey,
                            kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service(), kPrefServiceFactoryUnittestOtherInitialKey);
  EXPECT_EQ(
      kPrefServiceFactoryUnittestUpdatedValue,
      pref_service()->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
}

TEST_F(PrefServiceFactoryTest, MultipleConnectionsFromSingleClient) {
  Create();
  CreateForeign();
  Create();
  CreateForeign();
}

// Check that defaults set by one client are correctly shared to the other
// client.
TEST_F(PrefServiceFactoryTest, MultipleClients_Defaults) {
  std::unique_ptr<PrefService> pref_service, pref_service2;
  {
    base::RunLoop run_loop;
    auto done_closure = base::BarrierClosure(2, run_loop.QuitClosure());

    auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
    pref_registry->RegisterIntegerPref(kPrefServiceFactoryUnittestKey,
                                       kPrefServiceFactoryUnittestInitialValue,
                                       PrefRegistry::PUBLIC);
    pref_registry->RegisterForeignPref(kPrefServiceFactoryUnittestOtherKey);
    auto pref_registry2 = base::MakeRefCounted<PrefRegistrySimple>();
    pref_registry2->RegisterForeignPref(kPrefServiceFactoryUnittestKey);
    pref_registry2->RegisterIntegerPref(kPrefServiceFactoryUnittestOtherKey,
                                        kPrefServiceFactoryUnittestInitialValue,
                                        PrefRegistry::PUBLIC);
    CreateAsync(std::move(pref_registry), connector(), done_closure,
                &pref_service);
    CreateAsync(std::move(pref_registry2), other_client_connector(),
                done_closure, &pref_service2);
    run_loop.Run();
  }

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestOtherKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestOtherKey));
}

// Check that read-only pref store changes are observed.
TEST_F(PrefServiceFactoryTest, ReadOnlyPrefStore) {
  auto pref_service = Create();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));

  below_user_prefs_pref_store()->SetValue(
      kPrefServiceFactoryUnittestKey,
      std::make_unique<base::Value>(kPrefServiceFactoryUnittestUpdatedValue),
      0);
  WaitForPrefChange(pref_service.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  pref_service->SetInteger(kPrefServiceFactoryUnittestKey, 3);
  EXPECT_EQ(3, pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  above_user_prefs_pref_store()->SetValue(kPrefServiceFactoryUnittestKey,
                                          std::make_unique<base::Value>(4), 0);
  WaitForPrefChange(pref_service.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(4, pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
}

// Check that updates to read-only pref stores are correctly layered.
TEST_F(PrefServiceFactoryTest, ReadOnlyPrefStore_Layering) {
  auto pref_service = Create();

  above_user_prefs_pref_store()->SetValue(
      kPrefServiceFactoryUnittestKey,
      std::make_unique<base::Value>(kPrefServiceFactoryUnittestInitialValue),
      0);
  WaitForPrefChange(pref_service.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));

  below_user_prefs_pref_store()->SetValue(
      kPrefServiceFactoryUnittestKey,
      std::make_unique<base::Value>(kPrefServiceFactoryUnittestUpdatedValue),
      0);
  // This update is needed to check that the change to
  // kPrefServiceFactoryUnittestKey has propagated even though we will not
  // observe it change.
  below_user_prefs_pref_store()->SetValue(
      kPrefServiceFactoryUnittestOtherKey,
      std::make_unique<base::Value>(kPrefServiceFactoryUnittestUpdatedValue),
      0);
  WaitForPrefChange(pref_service.get(), kPrefServiceFactoryUnittestOtherKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
}

// Check that writes to user prefs are correctly layered with read-only
// pref stores.
TEST_F(PrefServiceFactoryTest, ReadOnlyPrefStore_UserPrefStoreLayering) {
  auto pref_service = Create();

  above_user_prefs_pref_store()->SetValue(kPrefServiceFactoryUnittestKey,
                                          std::make_unique<base::Value>(2), 0);
  WaitForPrefChange(pref_service.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(2, pref_service->GetInteger(kPrefServiceFactoryUnittestKey));

  pref_service->SetInteger(kPrefServiceFactoryUnittestKey, 3);
  EXPECT_EQ(2, pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
}

void Fail(PrefService* pref_service) {
  FAIL() << "Unexpected change notification: "
         << *pref_service->GetDictionary(
                kPrefServiceFactoryUnittestDictionaryKey);
}

TEST_F(PrefServiceFactoryTest, MultipleClients_SubPrefUpdates_Basic) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();

  void (*updates[])(ScopedDictionaryPrefUpdate*) = {
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetInteger("path.to.integer", 1);
        int out = 0;
        ASSERT_TRUE((*update)->GetInteger("path.to.integer", &out));
        EXPECT_EQ(1, out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetKey("key.for.integer", base::Value(2));
        int out = 0;
        ASSERT_TRUE(
            (*update)->GetIntegerWithoutPathExpansion("key.for.integer", &out));
        EXPECT_EQ(2, out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetDouble("path.to.double", 3);
        double out = 0;
        ASSERT_TRUE((*update)->GetDouble("path.to.double", &out));
        EXPECT_EQ(3, out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetKey("key.for.double", base::Value(4.0));
        double out = 0;
        ASSERT_TRUE(
            (*update)->GetDoubleWithoutPathExpansion("key.for.double", &out));
        EXPECT_EQ(4, out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetBoolean("path.to.boolean", true);
        bool out = 0;
        ASSERT_TRUE((*update)->GetBoolean("path.to.boolean", &out));
        EXPECT_TRUE(out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetKey("key.for.boolean", base::Value(false));
        bool out = 0;
        ASSERT_TRUE(
            (*update)->GetBooleanWithoutPathExpansion("key.for.boolean", &out));
        EXPECT_FALSE(out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetString("path.to.string", "hello");
        std::string out;
        ASSERT_TRUE((*update)->GetString("path.to.string", &out));
        EXPECT_EQ("hello", out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetKey("key.for.string", base::Value("prefs!"));
        std::string out;
        ASSERT_TRUE(
            (*update)->GetStringWithoutPathExpansion("key.for.string", &out));
        EXPECT_EQ("prefs!", out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetString("path.to.string16", base::ASCIIToUTF16("hello"));
        base::string16 out;
        ASSERT_TRUE((*update)->GetString("path.to.string16", &out));
        EXPECT_EQ(base::ASCIIToUTF16("hello"), out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        (*update)->SetKey("key.for.string16",
                          base::Value(base::ASCIIToUTF16("prefs!")));
        base::string16 out;
        ASSERT_TRUE(
            (*update)->GetStringWithoutPathExpansion("key.for.string16", &out));
        EXPECT_EQ(base::ASCIIToUTF16("prefs!"), out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        base::ListValue list;
        list.AppendInteger(1);
        list.AppendDouble(2);
        list.AppendBoolean(true);
        list.AppendString("four");
        (*update)->Set("path.to.list", list.CreateDeepCopy());
        const base::ListValue* out = nullptr;
        ASSERT_TRUE((*update)->GetList("path.to.list", &out));
        EXPECT_EQ(list, *out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        base::ListValue list;
        list.AppendInteger(1);
        list.AppendDouble(2);
        list.AppendBoolean(true);
        list.AppendString("four");
        (*update)->SetWithoutPathExpansion("key.for.list",
                                           list.CreateDeepCopy());
        const base::ListValue* out = nullptr;
        ASSERT_TRUE(
            (*update)->GetListWithoutPathExpansion("key.for.list", &out));
        EXPECT_EQ(list, *out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        base::DictionaryValue dict;
        dict.SetInteger("int", 1);
        dict.SetDouble("double", 2);
        dict.SetBoolean("bool", true);
        dict.SetString("string", "four");
        (*update)->Set("path.to.dict", dict.CreateDeepCopy());
        const base::DictionaryValue* out = nullptr;
        ASSERT_TRUE((*update)->GetDictionary("path.to.dict", &out));
        EXPECT_EQ(dict, *out);
      },
      [](ScopedDictionaryPrefUpdate* update) {
        base::DictionaryValue dict;
        dict.SetInteger("int", 1);
        dict.SetDouble("double", 2);
        dict.SetBoolean("bool", true);
        dict.SetString("string", "four");
        (*update)->SetWithoutPathExpansion("key.for.dict",
                                           dict.CreateDeepCopy());
        const base::DictionaryValue* out = nullptr;
        ASSERT_TRUE(
            (*update)->GetDictionaryWithoutPathExpansion("key.for.dict", &out));
        EXPECT_EQ(dict, *out);
      },
  };
  int current_value = kPrefServiceFactoryUnittestInitialValue + 1;
  for (auto& mutation : updates) {
    base::Value expected_value;
    {
      ScopedDictionaryPrefUpdate update(
          pref_service.get(), kPrefServiceFactoryUnittestDictionaryKey);
      EXPECT_EQ(update->AsConstDictionary()->empty(), update->empty());
      EXPECT_EQ(update->AsConstDictionary()->size(), update->size());
      mutation(&update);
      EXPECT_EQ(update->AsConstDictionary()->empty(), update->empty());
      EXPECT_EQ(update->AsConstDictionary()->size(), update->size());
      expected_value = update->AsConstDictionary()->Clone();
    }

    EXPECT_EQ(expected_value, *pref_service->GetDictionary(
                                  kPrefServiceFactoryUnittestDictionaryKey));
    WaitForPrefChange(pref_service2.get(),
                      kPrefServiceFactoryUnittestDictionaryKey);
    EXPECT_EQ(expected_value, *pref_service2->GetDictionary(
                                  kPrefServiceFactoryUnittestDictionaryKey));

    {
      // Apply the same mutation again. Each mutation should be idempotent so
      // should not trigger a notification.
      ScopedDictionaryPrefUpdate update(
          pref_service.get(), kPrefServiceFactoryUnittestDictionaryKey);
      mutation(&update);
      EXPECT_EQ(expected_value, *update->AsConstDictionary());
    }
    {
      // Watch for an unexpected change to
      // kPrefServiceFactoryUnittestDictionaryKey.
      PrefChangeRegistrar registrar;
      registrar.Init(pref_service2.get());
      registrar.Add(kPrefServiceFactoryUnittestDictionaryKey,
                    base::Bind(&Fail, pref_service2.get()));

      // Make and wait for a change to another pref to ensure an unexpected
      // change to kPrefServiceFactoryUnittestDictionaryKey is detected.
      pref_service->SetInteger(kPrefServiceFactoryUnittestKey, ++current_value);
      WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestKey);
    }
  }
}

TEST_F(PrefServiceFactoryTest, MultipleClients_SubPrefUpdates_Erase) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();
  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->SetInteger("path.to.integer", 1);
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  EXPECT_FALSE(
      pref_service2->GetDictionary(kPrefServiceFactoryUnittestDictionaryKey)
          ->empty());

  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    ASSERT_TRUE(update->RemovePath("path.to.integer", nullptr));
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  EXPECT_TRUE(
      pref_service2->GetDictionary(kPrefServiceFactoryUnittestDictionaryKey)
          ->empty());
}

TEST_F(PrefServiceFactoryTest, MultipleClients_SubPrefUpdates_ClearDictionary) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();

  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->SetInteger("path.to.integer", 1);
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  EXPECT_FALSE(
      pref_service2->GetDictionary(kPrefServiceFactoryUnittestDictionaryKey)
          ->empty());

  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->Clear();
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  EXPECT_TRUE(
      pref_service2->GetDictionary(kPrefServiceFactoryUnittestDictionaryKey)
          ->empty());
}

TEST_F(PrefServiceFactoryTest,
       MultipleClients_SubPrefUpdates_ClearEmptyDictionary) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();

  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->SetInteger(kPrefServiceFactoryUnittestKey,
                       kPrefServiceFactoryUnittestInitialValue);
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->Remove(kPrefServiceFactoryUnittestKey, nullptr);
  }
  WaitForPrefChange(pref_service2.get(),
                    kPrefServiceFactoryUnittestDictionaryKey);
  EXPECT_TRUE(
      pref_service2->GetDictionary(kPrefServiceFactoryUnittestDictionaryKey)
          ->empty());

  {
    ScopedDictionaryPrefUpdate update(pref_service.get(),
                                      kPrefServiceFactoryUnittestDictionaryKey);
    update->Clear();
  }
  PrefChangeRegistrar registrar;
  registrar.Init(pref_service2.get());
  registrar.Add(kPrefServiceFactoryUnittestDictionaryKey,
                base::Bind(&Fail, pref_service2.get()));
  pref_service->SetInteger(kPrefServiceFactoryUnittestKey,
                           kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestKey);
}

class IncognitoPrefServiceFactoryTest
    : public PrefServiceFactoryTest,
      public testing::WithParamInterface<bool> {
 protected:
  void CustomizePrefDelegateAndFactory(PrefValueStore::Delegate* delegate,
                                       PrefServiceFactory* factory) override {
    scoped_refptr<PersistentPrefStore> overlay =
        base::MakeRefCounted<InMemoryPrefStore>();
    scoped_refptr<PersistentPrefStore> underlay =
        base::MakeRefCounted<InMemoryPrefStore>();
    const auto overlay_pref_names = GetOverlayPrefNames();
    delegate->InitIncognitoUserPrefs(overlay, underlay, overlay_pref_names);
    auto overlay_pref_store = base::MakeRefCounted<OverlayUserPrefStore>(
        overlay.get(), underlay.get());
    for (auto* overlay_pref_name : overlay_pref_names)
      overlay_pref_store->RegisterOverlayPref(overlay_pref_name);
    factory->set_user_prefs(std::move(overlay_pref_store));
  }

  std::vector<const char*> GetOverlayPrefNames() {
    if (GetParam())
      return {kPrefServiceFactoryUnittestInitialKey,
              kPrefServiceFactoryUnittestOtherInitialKey,
              kPrefServiceFactoryUnittestKey};
    return {};
  }
};

// Check that updates in one client eventually propagates to the other.
TEST_P(IncognitoPrefServiceFactoryTest, InternalAndExternalClients) {
  auto pref_service2 = Create();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service()->GetInteger(kPrefServiceFactoryUnittestInitialKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestInitialKey));
  EXPECT_EQ(
      kPrefServiceFactoryUnittestInitialValue,
      pref_service()->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
  EXPECT_EQ(
      kPrefServiceFactoryUnittestInitialValue,
      pref_service2->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
  pref_service()->SetInteger(kPrefServiceFactoryUnittestInitialKey,
                             kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestInitialKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestInitialKey));

  pref_service2->SetInteger(kPrefServiceFactoryUnittestOtherInitialKey,
                            kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service(), kPrefServiceFactoryUnittestOtherInitialKey);
  EXPECT_EQ(
      kPrefServiceFactoryUnittestUpdatedValue,
      pref_service()->GetInteger(kPrefServiceFactoryUnittestOtherInitialKey));
}

// Check that updates in one client eventually propagates to the other.
TEST_P(IncognitoPrefServiceFactoryTest, MultipleClients) {
  auto pref_service = Create();
  auto pref_service2 = CreateForeign();

  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service->GetInteger(kPrefServiceFactoryUnittestKey));
  EXPECT_EQ(kPrefServiceFactoryUnittestInitialValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestKey));
  pref_service->SetInteger(kPrefServiceFactoryUnittestKey,
                           kPrefServiceFactoryUnittestUpdatedValue);
  WaitForPrefChange(pref_service2.get(), kPrefServiceFactoryUnittestKey);
  EXPECT_EQ(kPrefServiceFactoryUnittestUpdatedValue,
            pref_service2->GetInteger(kPrefServiceFactoryUnittestKey));
}

INSTANTIATE_TEST_CASE_P(UnderlayOrOverlayPref,
                        IncognitoPrefServiceFactoryTest,
                        testing::Bool());

}  // namespace
}  // namespace prefs
