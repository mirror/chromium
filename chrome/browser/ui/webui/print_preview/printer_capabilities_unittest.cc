// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/test/values_test_util.h"
#include "chrome/browser/ui/webui/print_preview/printer_capabilities.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "printing/backend/test_print_backend.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"

namespace printing {

namespace {

std::unique_ptr<base::DictionaryValue> GetCapabilitiesFull() {
  std::unique_ptr<base::DictionaryValue> printer =
      std::make_unique<base::DictionaryValue>();
  std::unique_ptr<base::ListValue> list_media =
      std::make_unique<base::ListValue>();
  list_media->GetList().push_back(base::Value("Letter"));
  list_media->GetList().push_back(base::Value("A4"));
  std::unique_ptr<base::DictionaryValue> options =
      std::make_unique<base::DictionaryValue>();
  std::unique_ptr<base::ListValue> list_dpi =
      std::make_unique<base::ListValue>();
  list_dpi->GetList().push_back(base::Value(300));
  list_dpi->GetList().push_back(base::Value(600));
  options->SetList(printing::kOptionKey, std::move(list_dpi));
  printer->SetList("media_sizes", std::move(list_media));
  printer->SetDictionary("dpi", std::move(options));
  std::unique_ptr<base::Value> collate = std::make_unique<base::Value>(true);
  printer->Set("collate", std::move(collate));
  return printer;
}

}  // namespace

class PrinterCapabilitiesTest : public testing::Test {
 public:
  PrinterCapabilitiesTest() {}
  ~PrinterCapabilitiesTest() override {}

 protected:
  void SetUp() override {
    test_backend_ = base::MakeRefCounted<TestPrintBackend>();
    PrintBackend::SetPrintBackendForTesting(test_backend_.get());
  }

  void TearDown() override { test_backend_ = nullptr; }

  TestPrintBackend* print_backend() { return test_backend_.get(); }

 private:
  content::TestBrowserThreadBundle test_browser_threads_;
  scoped_refptr<TestPrintBackend> test_backend_;
};

// Verify that we don't crash for a missing printer and a nullptr is never
// returned.
TEST_F(PrinterCapabilitiesTest, NonNullForMissingPrinter) {
  PrinterBasicInfo basic_info;
  std::string printer_name = "missing_printer";

  std::unique_ptr<base::DictionaryValue> settings_dictionary =
      GetSettingsOnBlockingPool(printer_name, basic_info);

  ASSERT_TRUE(settings_dictionary);
}

TEST_F(PrinterCapabilitiesTest, ProvidedCapabilitiesUsed) {
  std::string printer_name = "test_printer";
  PrinterBasicInfo basic_info;
  auto caps = base::MakeUnique<PrinterSemanticCapsAndDefaults>();

  // set a capability
  caps->dpis = {gfx::Size(600, 600)};

  print_backend()->AddValidPrinter(printer_name, std::move(caps));

  std::unique_ptr<base::DictionaryValue> settings_dictionary =
      GetSettingsOnBlockingPool(printer_name, basic_info);

  // verify settings were created
  ASSERT_TRUE(settings_dictionary);

  // verify capabilities and have one entry
  base::DictionaryValue* cdd;
  ASSERT_TRUE(
      settings_dictionary->GetDictionary(printing::kPrinterCapabilities, &cdd));

  // read the CDD for the dpi attribute.
  base::DictionaryValue* caps_dict;
  ASSERT_TRUE(cdd->GetDictionary(printing::kPrinterKey, &caps_dict));
  EXPECT_TRUE(caps_dict->HasKey("dpi"));
}

// Ensure that the capabilities dictionary is present but empty if the backend
// doesn't return capabilities.
TEST_F(PrinterCapabilitiesTest, NullCapabilitiesExcluded) {
  std::string printer_name = "test_printer";
  PrinterBasicInfo basic_info;

  // return false when attempting to retrieve capabilities
  print_backend()->AddValidPrinter(printer_name, nullptr);

  std::unique_ptr<base::DictionaryValue> settings_dictionary =
      GetSettingsOnBlockingPool(printer_name, basic_info);

  // verify settings were created
  ASSERT_TRUE(settings_dictionary);

  // verify that capabilities is an empty dictionary
  base::DictionaryValue* caps_dict;
  ASSERT_TRUE(settings_dictionary->GetDictionary(printing::kPrinterCapabilities,
                                                 &caps_dict));
  EXPECT_TRUE(caps_dict->empty());
}

TEST_F(PrinterCapabilitiesTest, FullCddPassthrough) {
  base::DictionaryValue cdd;
  std::unique_ptr<base::DictionaryValue> printer = GetCapabilitiesFull();
  cdd.SetDictionary(printing::kPrinterKey, std::move(printer));

  auto cdd_out = printing::ValidateCddForPrintPreview(cdd);
  base::Value* printer_out = cdd_out->FindPath({printing::kPrinterKey});
  base::Value* media_out = printer_out->FindPath({"media_sizes"});
  ASSERT_TRUE(media_out && !media_out->GetList().empty());
  EXPECT_TRUE(media_out->GetList()[0].GetString() == "Letter");
  EXPECT_TRUE(media_out->GetList()[1].GetString() == "A4");
  base::Value* dpi_option_out = printer_out->FindPath({"dpi"});
  ASSERT_TRUE(dpi_option_out);
  base::Value* dpi_list_out = dpi_option_out->FindPath({"option"});
  ASSERT_TRUE(dpi_list_out && !dpi_list_out->GetList().empty());
  EXPECT_TRUE(dpi_list_out->GetList()[0].GetInt() == 300);
  EXPECT_TRUE(dpi_list_out->GetList()[1].GetInt() == 600);
  base::Value* collate_out = printer_out->FindPath({"collate"});
  ASSERT_TRUE(collate_out);
  EXPECT_TRUE(collate_out->GetBool());
}

TEST_F(PrinterCapabilitiesTest, FilterBadList) {
  base::DictionaryValue cdd;
  std::unique_ptr<base::DictionaryValue> printer = GetCapabilitiesFull();
  printer->RemovePath({"media_sizes"});
  std::unique_ptr<base::ListValue> list_media =
      std::make_unique<base::ListValue>();
  list_media->GetList().push_back(base::Value());
  list_media->GetList().push_back(base::Value());
  printer->SetList("media_sizes", std::move(list_media));
  cdd.SetDictionary(printing::kPrinterKey, std::move(printer));

  auto cdd_out = printing::ValidateCddForPrintPreview(cdd);
  base::Value* printer_out = cdd_out->FindPath({printing::kPrinterKey});
  base::Value* media_out = printer_out->FindPath({"media_sizes"});
  ASSERT_FALSE(media_out);
  base::Value* dpi_option_out = printer_out->FindPath({"dpi"});
  ASSERT_TRUE(dpi_option_out);
  base::Value* dpi_list_out = dpi_option_out->FindPath({"option"});
  ASSERT_TRUE(dpi_list_out && !dpi_list_out->GetList().empty());
  EXPECT_TRUE(dpi_list_out->GetList()[0].GetInt() == 300);
  EXPECT_TRUE(dpi_list_out->GetList()[1].GetInt() == 600);
  base::Value* collate_out = printer_out->FindPath({"collate"});
  ASSERT_TRUE(collate_out);
  EXPECT_TRUE(collate_out->GetBool());
}

TEST_F(PrinterCapabilitiesTest, FilterBadOptionOneElement) {
  base::DictionaryValue cdd;
  std::unique_ptr<base::DictionaryValue> printer = GetCapabilitiesFull();
  printer->RemovePath({"dpi"});
  std::unique_ptr<base::DictionaryValue> options =
      std::make_unique<base::DictionaryValue>();
  std::unique_ptr<base::ListValue> list_dpi =
      std::make_unique<base::ListValue>();
  list_dpi->GetList().push_back(base::Value());
  list_dpi->GetList().push_back(base::Value(600));
  options->SetList(printing::kOptionKey, std::move(list_dpi));
  printer->SetDictionary("dpi", std::move(options));
  cdd.SetDictionary(printing::kPrinterKey, std::move(printer));

  auto cdd_out = printing::ValidateCddForPrintPreview(cdd);
  base::Value* printer_out = cdd_out->FindPath({printing::kPrinterKey});
  base::Value* media_out = printer_out->FindPath({"media_sizes"});
  ASSERT_TRUE(media_out && !media_out->GetList().empty());
  EXPECT_TRUE(media_out->GetList()[0].GetString() == "Letter");
  EXPECT_TRUE(media_out->GetList()[1].GetString() == "A4");
  base::Value* dpi_option_out = printer_out->FindPath({"dpi"});
  ASSERT_TRUE(dpi_option_out);
  base::Value* dpi_list_out = dpi_option_out->FindPath({"option"});
  ASSERT_TRUE(dpi_list_out && !dpi_list_out->GetList().empty());
  EXPECT_TRUE(dpi_list_out->GetList()[0].GetInt() == 600);
  base::Value* collate_out = printer_out->FindPath({"collate"});
  ASSERT_TRUE(collate_out);
  EXPECT_TRUE(collate_out->GetBool());
}

TEST_F(PrinterCapabilitiesTest, FilterBadOptionAllElement) {
  base::DictionaryValue cdd;
  std::unique_ptr<base::DictionaryValue> printer = GetCapabilitiesFull();
  printer->RemovePath({"dpi"});
  std::unique_ptr<base::DictionaryValue> options =
      std::make_unique<base::DictionaryValue>();
  std::unique_ptr<base::ListValue> list_dpi =
      std::make_unique<base::ListValue>();
  list_dpi->GetList().push_back(base::Value());
  list_dpi->GetList().push_back(base::Value());
  options->SetList(printing::kOptionKey, std::move(list_dpi));
  printer->SetDictionary("dpi", std::move(options));
  cdd.SetDictionary(printing::kPrinterKey, std::move(printer));

  auto cdd_out = printing::ValidateCddForPrintPreview(cdd);
  base::Value* printer_out = cdd_out->FindPath({printing::kPrinterKey});
  base::Value* media_out = printer_out->FindPath({"media_sizes"});
  ASSERT_TRUE(media_out && !media_out->GetList().empty());
  EXPECT_TRUE(media_out->GetList()[0].GetString() == "Letter");
  EXPECT_TRUE(media_out->GetList()[1].GetString() == "A4");
  base::Value* dpi_option_out = printer_out->FindPath({"dpi"});
  ASSERT_FALSE(dpi_option_out);
  base::Value* collate_out = printer_out->FindPath({"collate"});
  ASSERT_TRUE(collate_out);
  EXPECT_TRUE(collate_out->GetBool());
}
}  // namespace printing
