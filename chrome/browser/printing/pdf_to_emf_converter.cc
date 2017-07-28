// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/pdf_to_emf_converter.h"

#include <stdint.h>
#include <windows.h>

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/chrome_utility_printing_messages.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/browser/utility_process_host_client.h"
#include "printing/emf_win.h"
#include "printing/pdf_render_settings.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;

namespace printing {

namespace {

class PdfConverterImpl;

void CloseFileOnBlockingTaskRunner(base::File temp_file) {
  base::ThreadRestrictions::AssertIOAllowed();
  temp_file.Close();
}

base::File CreateTempFile() {
  base::File file;
  base::FilePath path;
  if (!base::CreateTemporaryFile(&path)) {
    PLOG(ERROR) << "Failed to create temp file";
    return file;
  }
  file = base::File(path, base::File::FLAG_CREATE_ALWAYS |
                              base::File::FLAG_WRITE | base::File::FLAG_READ |
                              base::File::FLAG_DELETE_ON_CLOSE |
                              base::File::FLAG_TEMPORARY);
  if (!file.IsValid())
    PLOG(ERROR) << "Failed to open temp file" << path.value();
  return file;
}

base::File CreateTempPdfFile(
    const scoped_refptr<base::RefCountedMemory>& data) {
  base::ThreadRestrictions::AssertIOAllowed();

  base::File pdf_file = CreateTempFile();
  if (!pdf_file.IsValid() ||
      static_cast<int>(data->size()) !=
          pdf_file.WriteAtCurrentPos(data->front_as<char>(), data->size()) ||
      pdf_file.Seek(base::File::FROM_BEGIN, 0) == -1) {
    return base::File();
  }
  return pdf_file;
}

std::vector<char> ReadTempFile(base::File temp_file) {
  std::vector<char> data;
  int64_t size = temp_file.GetLength();
  if (size > 0) {
    bool seeked = temp_file.Seek(base::File::FROM_BEGIN, 0) != -1;
    if (seeked) {
      data.resize(size);
      if (temp_file.ReadAtCurrentPos(data.data(), data.size()) != size)
        data.clear();
    }
  }
  temp_file.Close();
  return data;
}

class EmfMetaFile : public MetafilePlayer {
 public:
  explicit EmfMetaFile(std::vector<char> data) : data_(std::move(data)) {
    CHECK(!data_.empty());
  }
  ~EmfMetaFile() override {}

 protected:
  // MetafilePlayer:
  bool SafePlayback(HDC hdc) const override;
  bool GetDataAsVector(std::vector<char>* buffer) const override;
  bool SaveTo(base::File* file) const override;

  bool LoadEmf(Emf* emf) const;

 private:
  std::vector<char> data_;

  DISALLOW_COPY_AND_ASSIGN(EmfMetaFile);
};

// Postscript metafile subclass to override SafePlayback.
class PostScriptMetaFile : public EmfMetaFile {
 public:
  explicit PostScriptMetaFile(std::vector<char> data)
      : EmfMetaFile(std::move(data)) {}
  ~PostScriptMetaFile() override {}

 private:
  // MetafilePlayer:
  bool SafePlayback(HDC hdc) const override;

  DISALLOW_COPY_AND_ASSIGN(PostScriptMetaFile);
};

// Class for converting PDF to another format for printing (Emf, Postscript).
// Class uses UI thread, IO thread and |blocking_task_runner_|.
// Internal workflow is following:
// 1. Create instance on the UI thread. (files_, settings_,)
// 2. Create pdf file on |blocking_task_runner_|.
// 3. Start utility process and start conversion on the IO thread.
// 4. Utility process returns page count.
// 5. For each page:
//   1. Clients requests page with file handle to a temp file.
//   2. Utility converts the page, save it to the file and reply.
//
// All these steps work sequentially, so no data should be accessed
// simultaneously by several threads.
class PdfConverterUtilityProcessHostClient
    : public content::UtilityProcessHostClient {
 public:
  PdfConverterUtilityProcessHostClient(
      base::WeakPtr<PdfConverterImpl> converter,
      const PdfRenderSettings& settings);

  void Start(const scoped_refptr<base::RefCountedMemory>& data,
             const PdfConverter::StartCallback& start_callback);

  void GetPage(int page_number,
               const PdfConverter::GetPageCallback& get_page_callback);

  void Stop();

  // UtilityProcessHostClient:
  void OnProcessCrashed(int exit_code) override;
  void OnProcessLaunchFailed(int exit_code) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // Needs to be public to handle ChromeUtilityHostMsg_PreCacheFontCharacters
  // sync message replies.
  bool Send(IPC::Message* msg);

 private:
  class GetPageCallbackData {
   public:
    GetPageCallbackData(int page_number, PdfConverter::GetPageCallback callback)
        : page_number_(page_number), callback_(callback) {}

    GetPageCallbackData(GetPageCallbackData&& other) {
      *this = std::move(other);
    }

    ~GetPageCallbackData() {
      if (file_.IsValid()) {
        base::PostTaskWithTraits(
            FROM_HERE,
            {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
             base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
            base::BindOnce(&CloseFileOnBlockingTaskRunner,
                           base::Passed(std::move(file_))));
      }
    }

    GetPageCallbackData& operator=(GetPageCallbackData&& rhs) {
      page_number_ = rhs.page_number_;
      callback_ = rhs.callback_;
      file_ = std::move(rhs.file_);
      return *this;
    }

    int page_number() const { return page_number_; }
    const PdfConverter::GetPageCallback& callback() const { return callback_; }
    base::File TakeFile() { return std::move(file_); }
    void SetFile(base::File file) { file_ = std::move(file); }

   private:
    int page_number_;

    PdfConverter::GetPageCallback callback_;
    base::File file_;

    DISALLOW_COPY_AND_ASSIGN(GetPageCallbackData);
  };

  ~PdfConverterUtilityProcessHostClient() override;

  // Set the process name
  base::string16 GetName() const;

  // Create a MetafilePlayer from page data.
  std::unique_ptr<MetafilePlayer> GetMetaFileFromData(std::vector<char> data);

  // Send the messages to Start, GetPage, and Stop.
  void SendStartMessage(IPC::PlatformFileForTransit transit);
  void SendGetPageMessage(int page_number, IPC::PlatformFileForTransit transit);
  void SendStopMessage();

  // Message handlers:
  void OnPageCount(int page_count);
  void OnPageDone(bool success, float scale_factor);

  void OnPageFileRead(int page_number,
                      float scale_factor,
                      PdfConverter::GetPageCallback callback,
                      std::vector<char> data);
  void OnFailed();
  void OnTempPdfReady(base::File pdf_file);
  void OnTempFileReady(GetPageCallbackData* callback_data,
                       base::File temp_file);

  // Additional message handler needed for Pdf to Emf
  void OnPreCacheFontCharacters(const LOGFONT& log_font,
                                const base::string16& characters);

  // Used to suppress callbacks after PdfConverter is deleted.
  base::WeakPtr<PdfConverterImpl> converter_;
  PdfRenderSettings settings_;

  // Document loaded callback.
  PdfConverter::StartCallback start_callback_;

  // Process host for IPC.
  base::WeakPtr<content::UtilityProcessHost> utility_process_host_;

  // Queue of callbacks for GetPage() requests. Utility process should reply
  // with PageDone in the same order as requests were received.
  // Use containers that keeps element pointers valid after push() and pop().
  using GetPageCallbacks = std::queue<GetPageCallbackData>;
  GetPageCallbacks get_page_callbacks_;

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PdfConverterUtilityProcessHostClient);
};

std::unique_ptr<MetafilePlayer>
PdfConverterUtilityProcessHostClient::GetMetaFileFromData(
    std::vector<char> data) {
  if (data.empty())
    return nullptr;

  if (settings_.mode == PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2 ||
      settings_.mode == PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3 ||
      settings_.mode == PdfRenderSettings::Mode::TEXTONLY) {
    return base::MakeUnique<PostScriptMetaFile>(std::move(data));
  }
  return base::MakeUnique<EmfMetaFile>(std::move(data));
}

class PdfConverterImpl : public PdfConverter {
 public:
  PdfConverterImpl();

  ~PdfConverterImpl() override;

  base::WeakPtr<PdfConverterImpl> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  void Start(const scoped_refptr<base::RefCountedMemory>& data,
             const PdfRenderSettings& conversion_settings,
             const StartCallback& start_callback);

  void GetPage(int page_number,
               const GetPageCallback& get_page_callback) override;

  // Helps to cancel callbacks if this object is destroyed.
  void RunCallback(const base::Closure& callback);

  void Start(
      const scoped_refptr<PdfConverterUtilityProcessHostClient>& utility_client,
      const scoped_refptr<base::RefCountedMemory>& data,
      const StartCallback& start_callback);

 private:
  scoped_refptr<PdfConverterUtilityProcessHostClient> utility_client_;

  base::WeakPtrFactory<PdfConverterImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PdfConverterImpl);
};

bool EmfMetaFile::SafePlayback(HDC hdc) const {
  Emf emf;
  return LoadEmf(&emf) && emf.SafePlayback(hdc);
}

bool EmfMetaFile::GetDataAsVector(std::vector<char>* buffer) const {
  NOTREACHED();
  return false;
}

bool EmfMetaFile::SaveTo(base::File* file) const {
  Emf emf;
  return LoadEmf(&emf) && emf.SaveTo(file);
}

bool EmfMetaFile::LoadEmf(Emf* emf) const {
  return emf->InitFromData(data_.data(), data_.size());
}

bool PostScriptMetaFile::SafePlayback(HDC hdc) const {
  Emf emf;
  if (!LoadEmf(&emf))
    return false;

  Emf::Enumerator emf_enum(emf, nullptr, nullptr);
  for (const Emf::Record& record : emf_enum) {
    auto* emf_record = record.record();
    if (emf_record->iType != EMR_GDICOMMENT)
      continue;

    const EMRGDICOMMENT* comment =
        reinterpret_cast<const EMRGDICOMMENT*>(emf_record);
    const char* data = reinterpret_cast<const char*>(comment->Data);
    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
    int ret = ExtEscape(hdc, PASSTHROUGH, 2 + *ptr, data, 0, nullptr);
    DCHECK_EQ(*ptr, ret);
  }
  return true;
}

PdfConverterUtilityProcessHostClient::PdfConverterUtilityProcessHostClient(
    base::WeakPtr<PdfConverterImpl> converter,
    const PdfRenderSettings& settings)
    : converter_(converter),
      settings_(settings),
      blocking_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {}

PdfConverterUtilityProcessHostClient::~PdfConverterUtilityProcessHostClient() {}

void PdfConverterUtilityProcessHostClient::Start(
    const scoped_refptr<base::RefCountedMemory>& data,
    const PdfConverter::StartCallback& start_callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&PdfConverterUtilityProcessHostClient::Start, this, data,
                   start_callback));
    return;
  }

  // Store callback before any OnFailed() call to make it called on failure.
  start_callback_ = start_callback;

  // NOTE: This process _must_ be sandboxed, otherwise the pdf dll will load
  // gdiplus.dll, change how rendering happens, and not be able to correctly
  // generate when sent to a metafile DC.
  utility_process_host_ = content::UtilityProcessHost::Create(
                              this, base::ThreadTaskRunnerHandle::Get())
                              ->AsWeakPtr();
  utility_process_host_->SetName(GetName());

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CreateTempPdfFile, data),
      base::BindOnce(&PdfConverterUtilityProcessHostClient::OnTempPdfReady,
                     this));
}

void PdfConverterUtilityProcessHostClient::OnTempPdfReady(base::File pdf_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!utility_process_host_ || !pdf_file.IsValid())
    return OnFailed();
  // Should reply with OnPageCount().
  SendStartMessage(IPC::TakePlatformFileForTransit(std::move(pdf_file)));
}

void PdfConverterUtilityProcessHostClient::OnPageCount(int page_count) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (start_callback_.is_null())
    return OnFailed();
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&PdfConverterImpl::RunCallback, converter_,
                                     base::Bind(start_callback_, page_count)));
  start_callback_.Reset();
}

void PdfConverterUtilityProcessHostClient::GetPage(
    int page_number,
    const PdfConverter::GetPageCallback& get_page_callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&PdfConverterUtilityProcessHostClient::GetPage, this,
                   page_number, get_page_callback));
    return;
  }

  // Store callback before any OnFailed() call to make it called on failure.
  get_page_callbacks_.push(GetPageCallbackData(page_number, get_page_callback));

  if (!utility_process_host_)
    return OnFailed();

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE, base::BindOnce(&CreateTempFile),
      base::BindOnce(&PdfConverterUtilityProcessHostClient::OnTempFileReady,
                     this, &get_page_callbacks_.back()));
}

void PdfConverterUtilityProcessHostClient::OnTempFileReady(
    GetPageCallbackData* callback_data,
    base::File temp_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!utility_process_host_ || !temp_file.IsValid())
    return OnFailed();
  IPC::PlatformFileForTransit transit =
      IPC::GetPlatformFileForTransit(temp_file.GetPlatformFile(), false);
  callback_data->SetFile(std::move(temp_file));
  // Should reply with OnPageDone().
  SendGetPageMessage(callback_data->page_number(), transit);
}

void PdfConverterUtilityProcessHostClient::OnPageDone(bool success,
                                                      float scale_factor) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (get_page_callbacks_.empty())
    return OnFailed();

  GetPageCallbackData& data = get_page_callbacks_.front();
  if (!success) {
    OnPageFileRead(data.page_number(), scale_factor, std::move(data.callback()),
                   std::vector<char>());
    get_page_callbacks_.pop();
    return;
  }

  base::File temp_file = data.TakeFile();
  if (!temp_file.IsValid())  // Unexpected message from utility process.
    return OnFailed();

  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ReadTempFile, base::Passed(std::move(temp_file))),
      base::BindOnce(&PdfConverterUtilityProcessHostClient::OnPageFileRead,
                     this, data.page_number(), scale_factor,
                     std::move(data.callback())));
  get_page_callbacks_.pop();
}

void PdfConverterUtilityProcessHostClient::OnPageFileRead(
    int page_number,
    float scale_factor,
    PdfConverter::GetPageCallback callback,
    std::vector<char> data) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<MetafilePlayer> file = GetMetaFileFromData(std::move(data));
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PdfConverterImpl::RunCallback, converter_,
                 base::Bind(callback, page_number, scale_factor,
                            base::Passed(std::move(file)))));
}

void PdfConverterUtilityProcessHostClient::Stop() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&PdfConverterUtilityProcessHostClient::Stop, this));
    return;
  }
  SendStopMessage();
}

void PdfConverterUtilityProcessHostClient::OnProcessCrashed(int exit_code) {
  OnFailed();
}

void PdfConverterUtilityProcessHostClient::OnProcessLaunchFailed(
    int exit_code) {
  OnFailed();
}

bool PdfConverterUtilityProcessHostClient::Send(IPC::Message* msg) {
  if (utility_process_host_)
    return utility_process_host_->Send(msg);
  delete msg;
  return false;
}

void PdfConverterUtilityProcessHostClient::OnFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!start_callback_.is_null())
    OnPageCount(0);
  while (!get_page_callbacks_.empty())
    OnPageDone(false, 0.0f);
  utility_process_host_.reset();
}


void PdfConverterUtilityProcessHostClient::OnPreCacheFontCharacters(
    const LOGFONT& font,
    const base::string16& str) {
  // TODO(scottmg): pdf/ppapi still require the renderer to be able to precache
  // GDI fonts (http://crbug.com/383227), even when using DirectWrite.
  // Eventually this shouldn't be added and should be moved to
  // FontCacheDispatcher too. http://crbug.com/356346.

  // First, comments from FontCacheDispatcher::OnPreCacheFont do apply here too.
  // Except that for True Type fonts,
  // GetTextMetrics will not load the font in memory.
  // The only way windows seem to load properly, it is to create a similar
  // device (like the one in which we print), then do an ExtTextOut,
  // as we do in the printing thread, which is sandboxed.
  HDC hdc = CreateEnhMetaFile(nullptr, nullptr, nullptr, nullptr);
  HFONT font_handle = CreateFontIndirect(&font);
  DCHECK(font_handle != nullptr);

  HGDIOBJ old_font = SelectObject(hdc, font_handle);
  DCHECK(old_font != nullptr);

  ExtTextOut(hdc, 0, 0, ETO_GLYPH_INDEX, 0, str.c_str(), str.length(), nullptr);

  SelectObject(hdc, old_font);
  DeleteObject(font_handle);

  HENHMETAFILE metafile = CloseEnhMetaFile(hdc);

  if (metafile)
    DeleteEnhMetaFile(metafile);
}

bool PdfConverterUtilityProcessHostClient::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PdfConverterUtilityProcessHostClient, message)
    IPC_MESSAGE_HANDLER(
        ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageCount, OnPageCount)
    IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageDone,
                        OnPageDone)
    IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_PreCacheFontCharacters,
                        OnPreCacheFontCharacters)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

base::string16 PdfConverterUtilityProcessHostClient::GetName() const {
  return l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_PDF_CONVERTOR_NAME);
}

void PdfConverterUtilityProcessHostClient::SendGetPageMessage(
    int page_number,
    IPC::PlatformFileForTransit transit) {
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles_GetPage(page_number,
                                                              transit));
}

void PdfConverterUtilityProcessHostClient::SendStartMessage(
    IPC::PlatformFileForTransit transit) {
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles(transit, settings_));
}

void PdfConverterUtilityProcessHostClient::SendStopMessage() {
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop());
}

// Pdf Converter Impl and subclasses
PdfConverterImpl::PdfConverterImpl() : weak_ptr_factory_(this) {}

PdfConverterImpl::~PdfConverterImpl() {
  if (utility_client_.get())
    utility_client_->Stop();
}

void PdfConverterImpl::Start(
      const scoped_refptr<PdfConverterUtilityProcessHostClient>& utility_client,
      const scoped_refptr<base::RefCountedMemory>& data,
      const StartCallback& start_callback) {
    DCHECK(!utility_client_);
    utility_client_ = utility_client;
    utility_client_->Start(data, start_callback);
}

void PdfConverterImpl::GetPage(int page_number,
                               const GetPageCallback& get_page_callback) {
  utility_client_->GetPage(page_number, get_page_callback);
}

void PdfConverterImpl::RunCallback(const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  callback.Run();
}

}  // namespace

PdfConverter::~PdfConverter() {}

// static
std::unique_ptr<PdfConverter> PdfConverter::StartPdfConverter(
    const scoped_refptr<base::RefCountedMemory>& data,
    const PdfRenderSettings& conversion_settings,
    const StartCallback& start_callback) {
  std::unique_ptr<PdfConverterImpl> converter =
      base::MakeUnique<PdfConverterImpl>();
  converter->Start(
      new PdfConverterUtilityProcessHostClient(converter->GetWeakPtr(),
                                               conversion_settings),
          data, start_callback);
  return std::move(converter);
}

}  // namespace printing
