// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_
#define COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/arc/arc_service_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace arc {
namespace internal {

// Calls Traits::GetDependencyList() or returns empty vector if it is not
// defined.
template <typename Traits>
auto GetDependencyList() -> decltype(Traits::GetDependencyList()) {
  return Traits::GetDependencyList();
}

template <typename Traits>
std::vector<KeyedServiceBaseFactory*> GetDependencyList() {
  return std::vector<KeyedServiceBaseFactory*>();
}

// Implementation of BrowserContextKeyedServiceFactory for ARC related
// services.
// How to use:
// Traits needs to define two members.
// - kName: the name of factory.
// - Service: the type of the ARC related service needs to be created.
// In addition, it can define another function GetDepencyList() to declare
// its dependent KeyedService classes.
//
// Example:
// In header.
//
// class ArcFooService : public KeyedService, ... {
//  public:
//   static ArcFooService* GetForBrowserContext(
//       content::BrowserContext* context);
//
//   ArcFooService(content::BrowserContext* context,
//                 ArcBridgeService* arc_bridge_service);
//   ... more declarations and definitions ...
// };
//
// In source.
//
// namespace arc {
// namespace {
//
// struct FactoryTraits {
//   static constexpr const char* kName = "ArcFooServiceFactory";
//   using Service = ArcFooService;
// };
// using ArcFooServiceFactory =
//     internal::ArcBrowserContextKeyedServiceFactory<FactoryTraits>;
//
// }  // namespace
//
// // static
// ArcFooService* ArcFooService::GetForBrowserContext(
//     content::BrowserContext* context) {
//   return ArcFooServiceFactory::GetForBrowserContext(context);
// }
//
// If the service has dependencies to other KeyedServices, FactoryTraits
// needs to declare it as follows:
//
// struct FactoryTraits {
//   static constexpr const char* kName = "ArcFooServiceFactory";
//   using Service = ArcFooService;
//
//   static std::vector<KeyedServiceBaseFactory*> GetDependencyList() {
//     return {OtherBrowserContextKeyedServiceFactory::GetInstance(),
//             YetAnotherBrowserContextKeyedServiceFactory::GetInstance()};
//   }
// };
//
// This header is intended to be included only from the source file directly.
template <typename Traits>
class ArcBrowserContextKeyedServiceFactoryImpl
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of the service for the given |context|,
  // or nullptr if |context| is not allowed to use ARC.
  // If the instance is not yet created, this creates a new service instance.
  static typename Traits::Service* GetForBrowserContext(
      content::BrowserContext* context) {
    return static_cast<typename Traits::Service*>(
        GetInstance()->GetServiceForBrowserContext(context, true /* create */));
  }

  // Returns the factory instance.
  static ArcBrowserContextKeyedServiceFactoryImpl<Traits>* GetInstance() {
    return base::Singleton<
        ArcBrowserContextKeyedServiceFactoryImpl<Traits>>::get();
  }

 private:
  friend struct base::DefaultSingletonTraits<
      ArcBrowserContextKeyedServiceFactoryImpl<Traits>>;

  ArcBrowserContextKeyedServiceFactoryImpl()
      : BrowserContextKeyedServiceFactory(
            Traits::kName,
            BrowserContextDependencyManager::GetInstance()) {
    for (auto* keyed_service_factory : GetDependencyList<Traits>())
      DependsOn(keyed_service_factory);
  }
  ~ArcBrowserContextKeyedServiceFactoryImpl() = default;

  // BrowserContextKeyedServiceFactory override:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override {
    auto* arc_service_manager = arc::ArcServiceManager::Get();

    // Practically, this is in testing case.
    if (!arc_service_manager) {
      VLOG(2) << "ArcServiceManager is not available.";
      return nullptr;
    }

    if (arc_service_manager->browser_context() != context) {
      VLOG(2) << "Non ARC allowed browser context.";
      return nullptr;
    }

    return new typename Traits::Service(
        context, arc_service_manager->arc_bridge_service());
  }

  DISALLOW_COPY_AND_ASSIGN(ArcBrowserContextKeyedServiceFactoryImpl);
};

}  // namespace internal
}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_
