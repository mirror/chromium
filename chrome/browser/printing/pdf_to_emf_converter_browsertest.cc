// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/in_process_browser_test.h"

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/printing/pdf_to_emf_converter.h"
#include "printing/emf_win.h"
#include "printing/metafile.h"
#include "printing/pdf_render_settings.h"

namespace printing {

namespace {

class PDFToEMFConverterBrowserTest : public InProcessBrowserTest {
 public:
  PDFToEMFConverterBrowserTest() {}
  ~PDFToEMFConverterBrowserTest() override {}
};

void StartCallbackImpl(base::Closure quit_closure,
                       int* page_count_out,
                       int page_count_in) {
  *page_count_out = page_count_in;
  quit_closure.Run();
}

void GetPageCallbackImpl(base::Closure quit_closure,
                         int* page_number_out,
                         std::unique_ptr<MetafilePlayer>* file_out,
                         int page_number_in,
                         float scale_factor,
                         std::unique_ptr<MetafilePlayer> file_in) {
  *page_number_out = page_number_in;
  *file_out = std::move(file_in);
  quit_closure.Run();
}

}  // namespace

IN_PROC_BROWSER_TEST_F(PDFToEMFConverterBrowserTest, TestFailure) {
  scoped_refptr<base::RefCountedStaticMemory> bad_pdf_data =
      base::MakeRefCounted<base::RefCountedStaticMemory>("0123456789", 10);

  base::RunLoop run_loop;
  int page_count = -1;
  std::unique_ptr<PdfConverter> pdf_converter = PdfConverter::StartPdfConverter(
      bad_pdf_data, PdfRenderSettings(),
      base::Bind(&StartCallbackImpl, run_loop.QuitClosure(), &page_count));
  run_loop.Run();
  EXPECT_EQ(0, page_count);
}

IN_PROC_BROWSER_TEST_F(PDFToEMFConverterBrowserTest, TestSuccess) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath test_data_dir;
  ASSERT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir));
  base::FilePath pdf_file = test_data_dir.AppendASCII(
      "chrome/test/data/printing/pdf_to_emf_test.pdf");
  std::string pdf_data_str;
  ASSERT_TRUE(base::ReadFileToString(pdf_file, &pdf_data_str));
  ASSERT_GT(pdf_data_str.length(), 0U);
  scoped_refptr<base::RefCountedString> pdf_data(
      base::RefCountedString::TakeString(&pdf_data_str));

  // A4 page format.
  PdfRenderSettings pdf_settings(gfx::Rect(0, 0, 1700, 2200), gfx::Point(0, 0),
                                 /*dpi=*/200, /*autorotate=*/false,
                                 PdfRenderSettings::Mode::NORMAL);

  constexpr int kNumberOfPages = 3;
  std::unique_ptr<PdfConverter> pdf_converter;
  {
    base::RunLoop run_loop;
    int page_count = -1;
    pdf_converter = PdfConverter::StartPdfConverter(
        pdf_data, pdf_settings,
        base::Bind(&StartCallbackImpl, run_loop.QuitClosure(), &page_count));
    run_loop.Run();
    ASSERT_EQ(kNumberOfPages, page_count);
  }

  for (int i = 0; i < kNumberOfPages; i++) {
    int page_number = -1;
    std::unique_ptr<MetafilePlayer> emf_file;
    base::RunLoop run_loop;
    pdf_converter->GetPage(
        i, base::Bind(&GetPageCallbackImpl, run_loop.QuitClosure(),
                      &page_number, &emf_file));
    run_loop.Run();
    ASSERT_EQ(i, page_number);

    // We have to save the EMF to a file as LazyEmf::GetDataAsVector() is not
    // implemented.
    base::FilePath actual_emf_file_path;
    ASSERT_TRUE(base::CreateTemporaryFile(&actual_emf_file_path));
    base::File actual_emf_file(
        actual_emf_file_path,
        base::File::Flags::FLAG_CREATE_ALWAYS | base::File::Flags::FLAG_WRITE);
    ASSERT_TRUE(emf_file->SaveTo(&actual_emf_file));
    actual_emf_file.Close();
    std::string actual_emf_data;
    ASSERT_TRUE(base::ReadFileToString(actual_emf_file_path, &actual_emf_data));
    ASSERT_GT(actual_emf_data.length(), 0U);

    base::FilePath expected_emf_file = test_data_dir.AppendASCII(
        "chrome/test/data/printing/pdf_to_emf_test_page_" +
        std::to_string(i + 1 /* page numbers start at 1 */) + ".emf");
    std::string expected_emf_data;
    ASSERT_TRUE(base::ReadFileToString(expected_emf_file, &expected_emf_data));
    ASSERT_GT(expected_emf_data.length(), 0U);

    // TODO(crbug.com/781403): the generated data can differ visually. Until
    // this is fixed only checking the output size and EMF header.
    ASSERT_EQ(expected_emf_data.length(), actual_emf_data.length());

    printing::Emf expected_emf;
    expected_emf.InitFromData(expected_emf_data.data(),
                              expected_emf_data.length());
    std::string expected_emf_header_data = expected_emf_data.substr(
        expected_emf_data.length() - expected_emf.GetDataSize());

    printing::Emf actual_emf;
    actual_emf.InitFromData(actual_emf_data.data(), actual_emf_data.length());
    std::string actual_emf_header_data = actual_emf_data.substr(
        actual_emf_data.length() - actual_emf.GetDataSize());

    EXPECT_EQ(expected_emf_header_data, actual_emf_header_data);
  }
}

}  // namespace printing
