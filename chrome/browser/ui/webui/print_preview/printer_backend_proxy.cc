// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/printer_backend_proxy.h"

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/stl_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/cloud_print/cloud_print_cdd_conversion.h"
#include "chrome/common/crash_keys.h"
#include "content/public/browser/browser_thread.h"
#include "printing/backend/print_backend.h"
#include "printing/backend/print_backend_consts.h"

namespace printing {

const char kPrinterId[] = "printerId";
const char kPrinterCapabilities[] = "capabilities";
namespace {

// Returns a dictionary representing printer capabilities as CDD.  Returns
// nullptr if a dictionary could not be generated.
std::unique_ptr<base::DictionaryValue>
GetPrinterCapabilitiesOnBlockingPoolThread(
    const std::string& device_name,
    scoped_refptr<PrintBackend> print_backend) {
  VLOG(1) << "Get printer capabilities start for " << device_name;
  crash_keys::ScopedPrinterInfo crash_key(
      print_backend->GetPrinterDriverInfo(device_name));

  PrinterSemanticCapsAndDefaults info;
  if (!print_backend->GetPrinterSemanticCapsAndDefaults(device_name, &info)) {
    LOG(WARNING) << "Failed to get capabilities for " << device_name;
    return nullptr;
  }

  std::unique_ptr<base::DictionaryValue> printer_capabilities =
      cloud_print::PrinterSemanticCapsAndDefaultsToCdd(info);
  if (!printer_capabilities) {
    LOG(WARNING) << "Failed to convert capabilities for " << device_name;
    return nullptr;
  }

  return printer_capabilities;
}

std::unique_ptr<base::DictionaryValue> GetSettings(
    const std::string& device_name,
    const PrinterBasicInfo& basic_info) {
  const auto printer_name_description =
      PrinterBackendProxy::GetPrinterNameAndDescription(basic_info);
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

  return printer_info;
}

PrinterList EnumeratePrintersAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<PrintBackend> print_backend(
      PrintBackend::CreateInstance(nullptr));

  PrinterList printer_list;
  print_backend->EnumeratePrinters(&printer_list);

  return printer_list;
}

std::string GetDefaultPrinterAsync() {
  base::ThreadRestrictions::AssertIOAllowed();
  scoped_refptr<printing::PrintBackend> print_backend(
      PrintBackend::CreateInstance(nullptr));

  std::string default_printer = print_backend->GetDefaultPrinterName();
  VLOG(1) << "Default Printer: " << default_printer;
  return default_printer;
}

// Default implementation of PrinterBackendProxy.  Makes calls directly to
// the print backend on the appropriate thread.
class PrinterBackendProxyDefault : public PrinterBackendProxy {
 public:
  PrinterBackendProxyDefault() {}

  ~PrinterBackendProxyDefault() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  void GetDefaultPrinter(const DefaultPrinterCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::Bind(&GetDefaultPrinterAsync), cb);
  }

  void EnumeratePrinters(const EnumeratePrintersCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::Bind(&EnumeratePrintersAsync), cb);
  }

  void ConfigurePrinterAndFetchCapabilities(
      const std::string& device_name,
      const PrinterSetupCallback& cb) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::Bind(&FetchCapabilitiesAsync, device_name), cb);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PrinterBackendProxyDefault);
};

}  // namespace

// Exposed for unit test.
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

  std::unique_ptr<base::DictionaryValue> printer_info =
      GetSettings(device_name, basic_info);

  auto capabilities =
      GetPrinterCapabilitiesOnBlockingPoolThread(device_name, print_backend);
  if (!capabilities)
    capabilities = base::MakeUnique<base::DictionaryValue>();
  printer_info->Set(kPrinterCapabilities, std::move(capabilities));
  return printer_info;
}

// static
std::pair<std::string, std::string>
PrinterBackendProxy::GetPrinterNameAndDescription(
    const PrinterBasicInfo& printer) {
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
  // On Mac, |printer.printer_description| specifies the printer name and
  // |printer.printer_name| specifies the device name / printer queue name.
  // Chrome OS emulates the Mac behavior.
  const std::string& real_name = printer.printer_description;
  std::string real_description;
  const auto it = printer.options.find(kDriverNameTagName);
  if (it != printer.options.end())
    real_description = it->second;
  return std::make_pair(real_name, real_description);
#else
  return std::make_pair(printer.printer_name, printer.printer_description);
#endif
}

std::unique_ptr<PrinterBackendProxy> PrinterBackendProxy::Create() {
  return base::MakeUnique<PrinterBackendProxyDefault>();
}

}  // namespace printing
