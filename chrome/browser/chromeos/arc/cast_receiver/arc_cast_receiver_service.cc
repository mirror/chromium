// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/cast_receiver/arc_cast_receiver_service.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/prefs/pref_change_registrar.h"

namespace arc {
namespace {

// Singleton factory for ArcCastReceiverService.
class ArcCastReceiverServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcCastReceiverService,
          ArcCastReceiverServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcCastReceiverServiceFactory";

  static ArcCastReceiverServiceFactory* GetInstance() {
    return base::Singleton<ArcCastReceiverServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcCastReceiverServiceFactory>;
  ArcCastReceiverServiceFactory() = default;
  ~ArcCastReceiverServiceFactory() override = default;
};

}  // namespace

// static
ArcCastReceiverService* ArcCastReceiverService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcCastReceiverServiceFactory::GetForBrowserContext(context);
}

ArcCastReceiverService::ArcCastReceiverService(content::BrowserContext* context,
                                               ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service), binding_(this) {
  arc_bridge_service_->cast_receiver()->AddObserver(this);

  pref_change_registrar_.reset(new PrefChangeRegistrar());
  pref_change_registrar_->Init(Profile::FromBrowserContext(context));
  pref_change_registrar_->Add(
      "CastReceiverEnabled",  // prefs::kCastReceiverEnabled,
      base::Bind(&ArcCastReceiverService::OnCastReceiverEnabledChanged,
                 base::Unretained(this)));
  pref_change_registrar_->Add(
      "CastReceiverName",  // prefs::kCastReceiverName,
      base::Bind(&ArcCastReceiverService::OnCastReceiverNameChanged,
                 base::Unretained(this)));
}

ArcCastReceiverService::~ArcCastReceiverService() {
  arc_bridge_service_->cast_receiver()->RemoveObserver(this);
}

void ArcCastReceiverService::OnInstanceReady() {
  mojom::CastReceiverInstance* cast_receiver_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->cast_receiver(), Init);
  DCHECK(cast_receiver_instance);
  mojom::CastReceiverHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));

  OnCastReceiverNameChanged();
  OnCastReceiverEnabledChanged();
}

void ArcCastReceiverService::OnCastReceiverEnabledChanged() {}

void ArcCastReceiverService::OnCastReceiverNameChanged() {}

}  // namespace arc
