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
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "printing/backend/mojo/printing.mojom-shared.h"
#include "printing/backend/print_backend.h"
#include "printing/backend/print_backend_consts.h"

namespace printing {
const char kPrinterId[] = "printerId";
const char kPrinterCapabilities[] = "capabilities";

namespace {
std::pair<std::string, std::string> GetPrinterNameAndDescription(
    const PrinterBasicInfo& printer) {
  return std::make_pair(printer.printer_name, printer.printer_description);
}

std::vector<printing::mojom::PrinterBasicInfoPtr> EnumeratePrintersAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<PrintBackend> print_backend(
      PrintBackend::CreateInstance(nullptr));

  PrinterList printer_list;
  print_backend->EnumeratePrinters(&printer_list);

  // Convert for mojo
  std::vector<printing::mojom::PrinterBasicInfoPtr> printers;
  for (const PrinterBasicInfo& printer_info : printer_list) {
    printing::mojom::PrinterBasicInfoPtr printer =
        printing::mojom::PrinterBasicInfo::New();
    printer->printer_name = printer_info.printer_name;
    printer->printer_description = printer_info.printer_description;
    printer->printer_status = printer_info.printer_status;
    printer->is_default = printer_info.is_default;
    for (auto option : printer_info.options) {
      printer->options[option.first] = option.second;
    }
    printers.push_back(std::move(printer));
  }
  return printers;
}

std::unique_ptr<base::DictionaryValue> FetchCapabilitiesAsync(
    const std::string& device_name) {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      printing::PrintBackend::CreateInstance(nullptr));

  VLOG(1) << "Get printer capabilities start for " << device_name;

  if (!print_backend->IsValidPrinter(device_name)) {
    LOG(WARNING) << "Invalid printer " << device_name;
    return nullptr;
  }

  PrinterBasicInfo basic_info;
  if (!print_backend->GetPrinterBasicInfo(device_name, &basic_info)) {
    return nullptr;
  }

  const auto printer_name_description =
      GetPrinterNameAndDescription(basic_info);
  const std::string& printer_name = printer_name_description.first;
  const std::string& printer_description = printer_name_description.second;

  auto printer_info = base::MakeUnique<base::DictionaryValue>();
  printer_info->SetString(kPrinterId, device_name);
  printer_info->SetString(kSettingPrinterName, printer_name);
  printer_info->SetString(kSettingPrinterDescription, printer_description);
  printer_info->SetBoolean(
      kCUPSEnterprisePrinter,
      base::ContainsKey(basic_info.options, kCUPSEnterprisePrinter) &&
          basic_info.options.at(kCUPSEnterprisePrinter) == kValueTrue);

  PrinterSemanticCapsAndDefaults info;
  if (!print_backend->GetPrinterSemanticCapsAndDefaults(device_name, &info)) {
    LOG(WARNING) << "Failed to get capabilities for " << device_name;
  }

  std::unique_ptr<base::DictionaryValue> printer_capabilities =
      cloud_print::PrinterSemanticCapsAndDefaultsToCdd(info);
  if (!printer_capabilities) {
    LOG(WARNING) << "Failed to convert capabilities for " << device_name;
  }

  if (!printer_capabilities) {
    printer_info->Set(kPrinterCapabilities,
                      base::MakeUnique<base::DictionaryValue>());
  } else {
    printer_info->Set(kPrinterCapabilities, std::move(printer_capabilities));
  }

  return printer_info;
}

std::string GetDefaultPrinterAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      PrintBackend::CreateInstance(nullptr));

  std::string default_printer = print_backend->GetDefaultPrinterName();
  VLOG(1) << "Default Printer: " << default_printer;
  return default_printer;
}

}  // namespace

PrintBackendHandlerImpl::PrintBackendHandlerImpl() = default;
PrintBackendHandlerImpl::~PrintBackendHandlerImpl() = default;

// static
void PrintBackendHandlerImpl::Create(
    chrome::mojom::PrintBackendHandlerRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<PrintBackendHandlerImpl>(),
                          std::move(request));
}

void PrintBackendHandlerImpl::EnumeratePrinters(
    const EnumeratePrintersCallback& callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&EnumeratePrintersAsync), callback);
}

void PrintBackendHandlerImpl::FetchCapabilities(
    const std::string& device_name,
    const FetchCapabilitiesCallback& callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&FetchCapabilitiesAsync, device_name), callback);
}

void PrintBackendHandlerImpl::GetDefaultPrinter(
    const GetDefaultPrinterCallback& callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&GetDefaultPrinterAsync), callback);
}

}  // namespace printing
