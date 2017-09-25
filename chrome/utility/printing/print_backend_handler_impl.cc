// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/printing/print_backend_handler_impl.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/common/cloud_print/cloud_print_cdd_conversion.h"
#include "chrome/common/printing/print_backend_handler.mojom.h"
#include "chrome/common/printing/printer_capabilities.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "printing/backend/print_backend.h"
#include "printing/backend/print_backend_consts.h"

namespace printing {

PrintBackendHandlerImpl::PrintBackendHandlerImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)),
      print_backend_(PrintBackend::CreateInstance(nullptr)) {}

PrintBackendHandlerImpl::~PrintBackendHandlerImpl() = default;

void PrintBackendHandlerImpl::EnumeratePrinters(
    EnumeratePrintersCallback callback) {
  base::ThreadRestrictions::AssertIOAllowed();
  CHECK(print_backend_);

  PrinterList printer_list;
  print_backend_->EnumeratePrinters(&printer_list);
  std::move(callback).Run(printer_list);
}

void PrintBackendHandlerImpl::FetchCapabilities(
    const std::string& device_name,
    FetchCapabilitiesCallback callback) {
  base::ThreadRestrictions::AssertIOAllowed();
  CHECK(print_backend_);

  VLOG(1) << "Get printer capabilities start for " << device_name;

  if (!print_backend_->IsValidPrinter(device_name)) {
    LOG(WARNING) << "Invalid printer " << device_name;
    std::move(callback).Run(nullptr);
  }

  printing::PrinterBasicInfo basic_info;
  if (!print_backend_->GetPrinterBasicInfo(device_name, &basic_info)) {
    std::move(callback).Run(nullptr);
  }

  auto printer_info = printing::GetSettingsOnBlockingPool(
      device_name, basic_info, print_backend_);

  std::move(callback).Run(std::move(printer_info));
}

void PrintBackendHandlerImpl::GetDefaultPrinter(
    GetDefaultPrinterCallback callback) {
  base::ThreadRestrictions::AssertIOAllowed();
  CHECK(print_backend_);

  std::string default_printer = print_backend_->GetDefaultPrinterName();
  VLOG(1) << "Default Printer: " << default_printer;

  std::move(callback).Run(default_printer);
}

}  // namespace printing
