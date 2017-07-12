// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_
#define COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_

#include "base/logging.h"
#include "base/macros.h"
#include "components/arc/arc_service_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace arc {
namespace internal {

// Implementation of BrowserContextKeyedServiceFactory for ARC related
// services. The ARC related BrowserContextKeyedService can make its factory
// class derived from this class.
//
// How to use:
// class ArcFooService : public KeyedService, ... {
//  public:
//   static ArcFooService* GetForBrowserContext(
//       content::BrowserContext* context);
//
//   ArcFooService(content::BrowserContext* context,
//                 ArcBridgeService* arc_bridge_service);
// };
//
// namespace {
// class ArcFooServiceFactory
//     : public internal::ArcBrowserContextKeyedServiceFactoryImpl<
//           ArcFooService, ArcFooServiceFactory> {
//  public:
//   static ArcFooServiceFactory* GetInstance() {
//     return base::Singleton<ArcFooServiceFactory>::get();
//   }
//
//  private:
//   friend struct base::DefaultSingletonTraits<ArcFooServiceFactory>;
//   ArcFooServiceFactory()
//       : ArcBrowserContextKeyedServiceFactoryImpl("ArcFooServiceFactory") {}
//   ~ArcFooServiceFactory() override = default;
// };
// }  // namespace
//
// ArcFooService* ArcFooService::GetForBrowserContext(
//     content::BrowserContext* context) {
//   return ArcFooServiceFactory::GetForBrowserContext(context);
// }
//
// If the service depends on other KeyedService, DependsOn() can be called in
// its ctor similar to other BrowserContextKeyedServiceFactory subclasses.
//
// This header is intended to be included only from the source file directly.
template <typename Service, typename Factory>
class ArcBrowserContextKeyedServiceFactoryImpl
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of the service for the given |context|,
  // or nullptr if |context| is not allowed to use ARC.
  // If the instance is not yet created, this creates a new service instance.
  static Service* GetForBrowserContext(content::BrowserContext* context) {
    return static_cast<Service*>(
        Factory::GetInstance()->GetServiceForBrowserContext(context,
                                                            true /* create */));
  }

 protected:
  explicit ArcBrowserContextKeyedServiceFactoryImpl(const char* name)
      : BrowserContextKeyedServiceFactory(
            name,
            BrowserContextDependencyManager::GetInstance()) {}
  ~ArcBrowserContextKeyedServiceFactoryImpl() override = default;

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

    return new Service(context, arc_service_manager->arc_bridge_service());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcBrowserContextKeyedServiceFactoryImpl);
};

}  // namespace internal
}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_BROWSER_CONTEXT_KEYED_SERVICE_FACTORY_IMPL_H_
