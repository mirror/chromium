// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/zeroconf_printer_detector.h"

#include <map>
#include <string>
#include <vector>

#include "base/md5.h"
#include "base/memory/ptr_util.h"
#include "base/observer_list_threadsafe.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/local_discovery/service_discovery_device_lister.h"
#include "chrome/browser/local_discovery/service_discovery_shared_client.h"
#include "chrome/browser/profiles/profile.h"

namespace chromeos {
namespace {

using local_discovery::ServiceDescription;
using local_discovery::ServiceDiscoveryDeviceLister;
using local_discovery::ServiceDiscoverySharedClient;

// Identifiers for the service names we listen on.  This also defines the
// *priority* of a service.  If we find that the same printer advertises for
// multiple of these service names, we'll use the lowest numbered service name.
//
// As we use these as indices, they must be contiguously numbered starting from
// 0.
enum ServiceType { kIppsService, kIppService, kNumServiceTypes };

// These must be kept in a 1:1 correspondence with ServiceType entries.
const char* kServiceNames[] = {"_ipps._tcp.local", "_ipp._tcp.local"};

static_assert(arraysize(kServiceNames) == kNumServiceTypes,
              "ServiceNames aren't in sync with ServiceTypes");

// A DetectedPrinter bundled with extra information needed to manage it within
// the zeroconf detector.
struct AugmentedDetectedPrinter {
  PrinterDetector::DetectedPrinter detected_printer;
  ServiceType service_type;
};

// These (including the default values) come from section 9.2 of the Bonjour
// Printing Spec v1.2, and the field names follow the spec definitions instead
// of the canonical Chromium style.
//
// Not all of these will necessarily be specified for a given printer.  Also, we
// only define the fields that we care about, others not listed here we just
// ignore.
class ParsedMetadata {
 public:
  std::string adminurl;
  std::string air = "none";
  std::string note;
  std::string pdl = "application/postscript";
  // We stray slightly from the spec for product.  In the bonjour spec, product
  // is always enclosed in parentheses because...reasons.  We strip out parens.
  std::string product;
  std::string rp;
  std::string ty;
  std::string usb_MDL;
  std::string usb_MFG;
  std::string UUID;

  // Parse out metadata from sd to fill this structure.
  explicit ParsedMetadata(const ServiceDescription& sd) {
    for (const std::string& m : sd.metadata) {
      size_t equal_pos = m.find('=');
      if (equal_pos == std::string::npos) {
        // Malformed, skip it.
        continue;
      }
      base::StringPiece key(m.data(), equal_pos);
      base::StringPiece value(m.data() + equal_pos + 1,
                              m.length() - (equal_pos + 1));
      if (key == "note") {
        note = value.as_string();
      } else if (key == "pdl") {
        pdl = value.as_string();
      } else if (key == "product") {
        // Strip parens; ignore anything not enclosed in parens as malformed.
        if (value.starts_with("(") && value.ends_with(")")) {
          product = value.substr(1, value.size() - 2).as_string();
        }
      } else if (key == "rp") {
        rp = value.as_string();
      } else if (key == "ty") {
        ty = value.as_string();
      } else if (key == "usb_MDL") {
        usb_MDL = value.as_string();
      } else if (key == "usb_MFG") {
        usb_MFG = value.as_string();
      } else if (key == "UUID") {
        UUID = value.as_string();
      }
    }
  }
  ParsedMetadata(const ParsedMetadata& other) = delete;
};

// Create a unique identifier for this printer based on the ServiceDescription.
// This is what is used to determine whether or not this is the same printer
// when seen again later.  We use an MD5 hash of fields we expect to be
// immutable.
//
// These ids are persistent in synced storage; if you change this function
// carelessly, you will create mismatches between users' stored printer
// configurations and the printers themselves.
//
// Note we explicitly *don't* use the service type in this hash, because the
// same printer may export multiple services (ipp and ipps), and we want them
// all to be considered the same printer.
std::string ZeroconfPrinterId(const ServiceDescription& service,
                              const ParsedMetadata& metadata) {
  base::MD5Context ctx;
  base::MD5Init(&ctx);
  base::MD5Update(&ctx, service.instance_name());
  base::MD5Update(&ctx, metadata.product);
  base::MD5Update(&ctx, metadata.UUID);
  base::MD5Update(&ctx, metadata.usb_MFG);
  base::MD5Update(&ctx, metadata.usb_MDL);
  base::MD5Update(&ctx, metadata.ty);
  base::MD5Update(&ctx, metadata.rp);
  base::MD5Digest digest;
  base::MD5Final(&digest, &ctx);
  return base::StringPrintf("zeroconf-%s",
                            base::MD5DigestToBase16(digest).c_str());
}

// Attempt to fill |detected_printer| using the information in
// |service_description| and |metadata|.  Return true on success, false on
// failure.
bool ConvertToPrinter(const ServiceDescription& service_description,
                      const ParsedMetadata& metadata,
                      ServiceType service_type,
                      AugmentedDetectedPrinter* augmented_printer) {
  // If we don't have the minimum information needed to attempt a setup, fail.
  // Also fail on a port of 0, as this is used to indicate that the service
  // doesn't *actually* exist, the device just wants to guard the name.
  if (service_description.service_name.empty() || metadata.ty.empty() ||
      service_description.ip_address.empty() ||
      (service_description.address.port() == 0)) {
    return false;
  }
  augmented_printer->service_type = service_type;

  PrinterDetector::DetectedPrinter& detected_printer =
      augmented_printer->detected_printer;
  Printer& printer = detected_printer.printer;
  printer.set_id(ZeroconfPrinterId(service_description, metadata));
  printer.set_uuid(metadata.UUID);
  printer.set_display_name(metadata.ty);
  printer.set_description(metadata.note);
  printer.set_make_and_model(metadata.product);
  const char* uri_protocol;
  if (service_description.service_type() ==
      base::StringPiece(kServiceNames[kIppService])) {
    uri_protocol = "ipp";
  } else if (service_description.service_type() ==
             base::StringPiece(kServiceNames[kIppsService])) {
    uri_protocol = "ipps";
  } else {
    // Since we only register for these services, we should never get back
    // a service other than the ones above.
    NOTREACHED() << "Zeroconf printer with unknown service type"
                 << service_description.service_type();
    return false;
  }
  printer.set_uri(base::StringPrintf(
      "%s://%s/%s", uri_protocol,
      service_description.address.ToString().c_str(), metadata.rp.c_str()));

  // Use an effective URI with a pre-resolved ip address and port, since CUPS
  // can't resolve these addresses in ChromeOS (crbug/626377).
  printer.set_effective_uri(base::StringPrintf(
      "%s://%s:%d/%s", uri_protocol,
      service_description.ip_address.ToString().c_str(),
      service_description.address.port(), metadata.rp.c_str()));

  // gather ppd identification candidates.
  if (!metadata.ty.empty()) {
    detected_printer.ppd_search_data.make_and_model.push_back(metadata.ty);
  }
  if (!metadata.product.empty()) {
    detected_printer.ppd_search_data.make_and_model.push_back(metadata.product);
  }
  if (!metadata.usb_MFG.empty() && !metadata.usb_MDL.empty()) {
    detected_printer.ppd_search_data.make_and_model.push_back(
        base::StringPrintf("%s %s", metadata.usb_MFG.c_str(),
                           metadata.usb_MDL.c_str()));
  }
  return true;
}

class ZeroconfPrinterDetectorImpl;

// Frustratingly, we don't have a good way in the Delegate interface to
// distinguish between the sources of cache flush calls if we're listening
// on multiple services.  This wrapper's sole purpose is to make it possible
// to determine the source of a given flush call.
class DelegateWrapper : public ServiceDiscoveryDeviceLister::Delegate {
 public:
  DelegateWrapper(ZeroconfPrinterDetectorImpl* parent, ServiceType service_type)
      : parent_(parent), service_type_(service_type) {}
  virtual ~DelegateWrapper() = default;

  // Delegate implementations.  These cannot be defined inline because they
  // need the ZeroconfPrinterDetectorImpl implementation.
  void OnDeviceChanged(bool added,
                       const ServiceDescription& service_description) override;
  void OnDeviceRemoved(const std::string& service_name) override;
  void OnDeviceCacheFlushed() override;

 private:
  ZeroconfPrinterDetectorImpl* parent_;
  ServiceType service_type_;
};

class ZeroconfPrinterDetectorImpl : public ZeroconfPrinterDetector {
 public:
  explicit ZeroconfPrinterDetectorImpl(Profile* profile)
      : discovery_client_(ServiceDiscoverySharedClient::GetInstance()),
        observer_list_(new base::ObserverListThreadSafe<Observer>()) {
    for (int i = 0; i < kNumServiceTypes; ++i) {
      device_lister_delegate_wrappers_.emplace_back(
          this, static_cast<ServiceType>(i));
      device_listers_.emplace_back(
          base::MakeUnique<ServiceDiscoveryDeviceLister>(
              &device_lister_delegate_wrappers_.back(), discovery_client_.get(),
              kServiceNames[i]));
      device_listers_.back()->Start();
      device_listers_.back()->DiscoverNewDevices();
    }
  }

  ~ZeroconfPrinterDetectorImpl() override {}

  std::vector<DetectedPrinter> GetPrinters() override {
    base::AutoLock auto_lock(printers_lock_);
    return GetPrintersLocked();
  }

  // When an observer is added, immediately schedule a callback with no printers
  // and a scan done callback.  Note that, since we're using
  // ThreadSafeObserverList the callbacks here are posted for later execution,
  // not executed immediately.
  void AddObserver(Observer* observer) override {
    observer_list_->AddObserver(observer);
    observer_list_->Notify(
        FROM_HERE, &PrinterDetector::Observer::OnPrintersFound, GetPrinters());
    // TODO(justincarlson) - Figure out a more intelligent way to figure out
    // when a scan is reasonably "done".
    observer_list_->Notify(FROM_HERE,
                           &PrinterDetector::Observer::OnPrinterScanComplete);
  }

  void RemoveObserver(Observer* observer) override {
    observer_list_->RemoveObserver(observer);
  }

  // ServiceDiscoveryDeviceLister::Delegate pseudo-implementations -- these
  // are the Delegate implementations with an additional ServiceType parameter
  // which is provided by DelegateWrapper.
  void OnDeviceChanged(ServiceType service_type,
                       bool added,
                       const ServiceDescription& service_description) {
    // We don't care if it was added or not; we generate an update either way.
    ParsedMetadata metadata(service_description);
    AugmentedDetectedPrinter printer;
    if (!ConvertToPrinter(service_description, metadata, service_type,
                          &printer)) {
      return;
    }

    std::string instance_name = service_description.instance_name();
    base::AutoLock auto_lock(printers_lock_);
    auto existing = printers_.find(instance_name);
    // Add it to our printers if it's new, or it's at least as high
    // a priority as the existing entry.
    if (existing == printers_.end() ||
        printer.service_type <= existing->second.service_type) {
      printers_[instance_name] = printer;
      observer_list_->Notify(FROM_HERE,
                             &PrinterDetector::Observer::OnPrintersFound,
                             GetPrintersLocked());
    }
  }

  void OnDeviceRemoved(ServiceType service_type,
                       const std::string& service_name) {
    // Leverage ServiceDescription parsing to pull out the instance name.
    ServiceDescription service_description;
    service_description.service_name = service_name;
    std::string instance_name = service_description.instance_name();
    base::AutoLock auto_lock(printers_lock_);
    auto it = printers_.find(instance_name);
    if (it != printers_.end() && it->second.service_type == service_type) {
      printers_.erase(it);
      observer_list_->Notify(FROM_HERE,
                             &PrinterDetector::Observer::OnPrintersFound,
                             GetPrintersLocked());
    }
  }

  // Clear out existing entries for this device, request a new discovery round.
  void OnDeviceCacheFlushed(ServiceType service_type) {
    base::AutoLock auto_lock(printers_lock_);
    // Selectively remove those printers that match service_type.
    for (auto it = printers_.begin(); it != printers_.end();) {
      if (it->second.service_type == service_type) {
        auto next = it;
        ++next;
        printers_.erase(it);
        it = next;
      } else {
        ++it;
      }
    }
    observer_list_->Notify(FROM_HERE,
                           &PrinterDetector::Observer::OnPrintersFound,
                           GetPrintersLocked());
    device_listers_[service_type]->DiscoverNewDevices();
  }

 private:
  // Gets the current canonical list of DetectedPrinter by pulling the highest
  // priority entry for a given instance from all entries.
  std::vector<DetectedPrinter> GetPrintersLocked() {
    printers_lock_.AssertAcquired();
    std::vector<DetectedPrinter> ret;
    ret.reserve(printers_.size());
    for (const auto& entry : printers_) {
      ret.push_back(entry.second.detected_printer);
    }
    return ret;
  }

  // Map from instance name to detected printer info.
  std::map<std::string, AugmentedDetectedPrinter> printers_;
  base::Lock printers_lock_;

  // Keep a reference to the shared device client around for the lifetime of
  // this object.
  scoped_refptr<ServiceDiscoverySharedClient> discovery_client_;
  std::vector<DelegateWrapper> device_lister_delegate_wrappers_;
  std::vector<std::unique_ptr<ServiceDiscoveryDeviceLister>> device_listers_;

  // Observers of this object.
  scoped_refptr<base::ObserverListThreadSafe<Observer>> observer_list_;
};

void DelegateWrapper::OnDeviceChanged(
    bool added,
    const ServiceDescription& service_description) {
  parent_->OnDeviceChanged(service_type_, added, service_description);
}
void DelegateWrapper::OnDeviceRemoved(const std::string& service_name) {
  parent_->OnDeviceRemoved(service_type_, service_name);
}

void DelegateWrapper::OnDeviceCacheFlushed() {
  parent_->OnDeviceCacheFlushed(service_type_);
}

}  // namespace

// static
//
std::unique_ptr<ZeroconfPrinterDetector> ZeroconfPrinterDetector::Create(
    Profile* profile) {
  return base::MakeUnique<ZeroconfPrinterDetectorImpl>(profile);
}

}  // namespace chromeos
