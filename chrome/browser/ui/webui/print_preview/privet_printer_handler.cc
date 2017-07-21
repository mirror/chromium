// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/privet_printer_handler.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/timer.h"
#include "chrome/browser/printing/cloud_print/privet_constants.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/chrome_switches.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ui/gfx/geometry/size.h"

// Timeout for searching for privet printers, in seconds.
const int kPrivetTimeoutSec = 5;

void PrivetPrinterHandler::FillPrinterDescription(
    const std::string& name,
    const cloud_print::DeviceDescription& description,
    bool has_local_printing,
    base::DictionaryValue* printer_value) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  printer_value->SetString("serviceName", name);
  printer_value->SetString("name", description.name);
  printer_value->SetBoolean("hasLocalPrinting", has_local_printing);
  printer_value->SetBoolean(
      "isUnregistered",
      description.id.empty() &&
          command_line->HasSwitch(switches::kEnablePrintPreviewRegisterPromos));
  printer_value->SetString("cloudID", description.id);
}

void PrivetPrinterHandler::LocalPrinterChanged(
    const std::string& name,
    bool has_local_printing,
    const cloud_print::DeviceDescription& description) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (has_local_printing ||
      command_line->HasSwitch(switches::kEnablePrintPreviewRegisterPromos)) {
    std::unique_ptr<base::DictionaryValue> printer_info =
        base::MakeUnique<base::DictionaryValue>();
    FillPrinterDescription(name, description, has_local_printing,
                           printer_info.get());
    base::ListValue printers;
    printers.Set(0, std::move(printer_info));
    get_printers_callback_.Run(printers, false);
  }
}

void PrivetPrinterHandler::LocalPrinterRemoved(const std::string& name) {}

void PrivetPrinterHandler::LocalPrinterCacheFlushed() {}

void PrivetPrinterHandler::StartPrivetLister(
    const scoped_refptr<local_discovery::ServiceDiscoverySharedClient>&
        client) {
  DCHECK(!service_discovery_client_.get() ||
         service_discovery_client_.get() == client.get());
  service_discovery_client_ = client;
  printer_lister_ = base::MakeUnique<cloud_print::PrivetLocalPrinterLister>(
      service_discovery_client_.get(), profile_->GetRequestContext(), this);
  privet_lister_timer_.reset(new base::OneShotTimer());
  privet_lister_timer_->Start(FROM_HERE,
                              base::TimeDelta::FromSeconds(kPrivetTimeoutSec),
                              this, &PrivetPrinterHandler::StopPrivetLister);
  printer_lister_->Start();
}

void PrivetPrinterHandler::StopPrivetLister() {
  privet_lister_timer_.reset();
  if (printer_lister_)
    printer_lister_->Stop();
  get_printers_callback_.Run(base::ListValue(), true);
}

PrivetPrinterHandler::PrivetPrinterHandler(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {}

PrivetPrinterHandler::~PrivetPrinterHandler() {}

void PrivetPrinterHandler::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PrivetPrinterHandler::StartGetPrinters(
    const PrinterHandler::GetPrintersCallback& callback) {
  using local_discovery::ServiceDiscoverySharedClient;
  scoped_refptr<ServiceDiscoverySharedClient> service_discovery =
      ServiceDiscoverySharedClient::GetInstance();
  get_printers_callback_ = callback;
  StartPrivetLister(service_discovery);
}

void PrivetPrinterHandler::StartGetCapability(
    const std::string& destination_id,
    const PrinterHandler::GetCapabilityCallback& callback) {
  if (CreatePrivetHTTP(
          destination_id,
          base::Bind(&PrivetPrinterHandler::PrivetCapabilitiesUpdateClient,
                     weak_ptr_factory_.GetWeakPtr(), callback))) {
    return;
  }
  callback.Run(base::DictionaryValue());
}

void PrivetPrinterHandler::PrivetCapabilitiesUpdateClient(
    const PrinterHandler::GetCapabilityCallback& callback,
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!PrivetUpdateClient(std::move(http_client))) {
    callback.Run(base::DictionaryValue());
    return;
  }

  privet_capabilities_operation_ =
      privet_http_client_->CreateCapabilitiesOperation(
          base::Bind(&PrivetPrinterHandler::OnPrivetCapabilities,
                     weak_ptr_factory_.GetWeakPtr(), callback));
  privet_capabilities_operation_->Start();
}

bool PrivetPrinterHandler::CreatePrivetHTTP(
    const std::string& name,
    const cloud_print::PrivetHTTPAsynchronousFactory::ResultCallback&
        callback) {
  const cloud_print::DeviceDescription* device_description =
      printer_lister_ ? printer_lister_->GetDeviceDescription(name) : NULL;

  if (!device_description)
    return false;

  privet_http_factory_ =
      cloud_print::PrivetHTTPAsynchronousFactory::CreateInstance(
          profile_->GetRequestContext());
  privet_http_resolution_ = privet_http_factory_->CreatePrivetHTTP(name);
  privet_http_resolution_->Start(device_description->address, callback);

  return true;
}

bool PrivetPrinterHandler::PrivetUpdateClient(
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!http_client) {
    privet_http_resolution_.reset();
    return false;
  }

  privet_local_print_operation_.reset();
  privet_capabilities_operation_.reset();
  privet_http_client_ =
      cloud_print::PrivetV1HTTPClient::CreateDefault(std::move(http_client));

  privet_http_resolution_.reset();

  return true;
}

void PrivetPrinterHandler::OnPrivetCapabilities(
    const PrinterHandler::GetCapabilityCallback& callback,
    const base::DictionaryValue* capabilities) {
  std::string name = privet_capabilities_operation_->GetHTTPClient()->GetName();

  if (!capabilities || capabilities->HasKey(cloud_print::kPrivetKeyError) ||
      !printer_lister_) {
    callback.Run(base::DictionaryValue());
    return;
  }

  const cloud_print::DeviceDescription* description =
      printer_lister_->GetDeviceDescription(name);

  if (!description) {
    callback.Run(base::DictionaryValue());
    return;
  }

  std::unique_ptr<base::DictionaryValue> printer_info =
      base::MakeUnique<base::DictionaryValue>();
  FillPrinterDescription(name, *description, true, printer_info.get());
  base::DictionaryValue printer_info_and_caps;
  printer_info_and_caps.SetDictionary("printer", std::move(printer_info));
  std::unique_ptr<base::DictionaryValue> capabilities_copy =
      capabilities->CreateDeepCopy();
  printer_info_and_caps.SetDictionary("capabilities",
                                      std::move(capabilities_copy));
  callback.Run(printer_info_and_caps);
  privet_capabilities_operation_.reset();
}

void PrivetPrinterHandler::PrivetLocalPrintUpdateClient(
    const PrinterHandler::PrintCallback& callback,
    const base::string16& job_title,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    std::string print_ticket,
    std::string capabilities,
    gfx::Size page_size,
    std::unique_ptr<cloud_print::PrivetHTTPClient> http_client) {
  if (!PrivetUpdateClient(std::move(http_client))) {
    callback.Run(false, base::Value(-1));
    return;
  }
  print_callback_ = callback;
  StartPrivetLocalPrint(job_title, print_data, print_ticket, capabilities,
                        page_size);
}

void PrivetPrinterHandler::StartPrivetLocalPrint(
    const base::string16& job_title,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    const std::string& print_ticket,
    const std::string& capabilities,
    const gfx::Size& page_size) {
  privet_local_print_operation_ =
      privet_http_client_->CreateLocalPrintOperation(this);

  privet_local_print_operation_->SetTicket(print_ticket);
  privet_local_print_operation_->SetCapabilities(capabilities);
  privet_local_print_operation_->SetJobname(base::UTF16ToUTF8(job_title));
  privet_local_print_operation_->SetPageSize(page_size);
  privet_local_print_operation_->SetData(print_data);

  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfileIfExists(profile_);

  if (signin_manager) {
    privet_local_print_operation_->SetUsername(
        signin_manager->GetAuthenticatedAccountInfo().email);
  }

  privet_local_print_operation_->Start();
}

void PrivetPrinterHandler::StartPrint(
    const std::string& destination_id,
    const std::string& capability,
    const base::string16& job_title,
    const std::string& ticket_json,
    const gfx::Size& page_size,
    const scoped_refptr<base::RefCountedBytes>& print_data,
    const PrinterHandler::PrintCallback& callback) {
  if (!CreatePrivetHTTP(
          destination_id,
          base::Bind(&PrivetPrinterHandler::PrivetLocalPrintUpdateClient,
                     weak_ptr_factory_.GetWeakPtr(), callback, job_title,
                     print_data, ticket_json, capability, page_size))) {
    callback.Run(false, base::Value(-1));
  }
}

void PrivetPrinterHandler::StartGrantPrinterAccess(
    const std::string& printer_id,
    const PrinterHandler::GetPrinterInfoCallback& callback) {
  NOTIMPLEMENTED();
}

void PrivetPrinterHandler::OnPrivetPrintingDone(
    const cloud_print::PrivetLocalPrintOperation* print_operation) {
  if (!print_callback_)
    NOTREACHED();
  print_callback_.Run(true, base::Value());
}

void PrivetPrinterHandler::OnPrivetPrintingError(
    const cloud_print::PrivetLocalPrintOperation* print_operation,
    int http_code) {
  if (!print_callback_)
    NOTREACHED();
  print_callback_.Run(false, base::Value(http_code));
}
