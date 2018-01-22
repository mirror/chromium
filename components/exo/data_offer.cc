// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/exo/data_offer_delegate.h"
#include "components/exo/data_offer_observer.h"
#include "components/exo/file_helper.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/dragdrop/file_info.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "url/gurl.h"

namespace exo {
namespace {

constexpr char kTextMimeTypeUtf8[] = "text/plain;charset=utf-8";
constexpr char kUriListSeparator[] = "\r\n";

class RefCountedString16 : public base::RefCountedMemory {
 public:
  static scoped_refptr<RefCountedString16> TakeString(
      base::string16&& to_destroy) {
    scoped_refptr<RefCountedString16> self(new RefCountedString16);
    to_destroy.swap(self->data_);
    return self;
  }

  // Overridden from base::RefCountedMemory:
  const unsigned char* front() const override {
    return reinterpret_cast<const unsigned char*>(data_.data());
  }
  size_t size() const override { return data_.size() * sizeof(base::char16); }

 protected:
  ~RefCountedString16() override {}

 private:
  base::string16 data_;
};

void WriteFileDescriptor(base::ScopedFD fd,
                         scoped_refptr<base::RefCountedMemory> memory) {
  if (!base::WriteFileDescriptor(fd.get(),
                                 reinterpret_cast<const char*>(memory->front()),
                                 memory->size()))
    DLOG(ERROR) << "Failed to write drop data";
}

// NOTE: This function relies on the pickle construction logic in
// src/content/browser/web_contents/web_contents_view_aura.cc.
bool ReadFileUrlsFromPickle(const base::Pickle& pickle,
                            std::vector<GURL>* file_urls) {
  base::PickleIterator iter(pickle);
  uint32_t num_files = 0;
  if (!iter.ReadUInt32(&num_files)) {
    return false;
  }
  for (uint32_t i = 0; i < num_files; ++i) {
    std::string url_string;
    int64_t size = 0;
    std::string filesystem_id;
    if (!iter.ReadString(&url_string) || !iter.ReadInt64(&size) ||
        !iter.ReadString(&filesystem_id)) {
      return false;
    }
    file_urls->push_back(GURL(url_string));
  }
  return true;
}

const ui::Clipboard::FormatType& GetFileSystemFileFormatType() {
  static const char kFormatString[] = "chromium/x-file-system-files";
  CR_DEFINE_STATIC_LOCAL(ui::Clipboard::FormatType, format,
                         (ui::Clipboard::GetFormatType(kFormatString)));
  return format;
}

}  // namespace

DataOffer::DataOffer(DataOfferDelegate* delegate) : delegate_(delegate) {}

DataOffer::~DataOffer() {
  delegate_->OnDataOfferDestroying(this);
  for (DataOfferObserver& observer : observers_) {
    observer.OnDataOfferDestroying(this);
  }
}

void DataOffer::AddObserver(DataOfferObserver* observer) {
  observers_.AddObserver(observer);
}

void DataOffer::RemoveObserver(DataOfferObserver* observer) {
  observers_.RemoveObserver(observer);
}

void DataOffer::Accept(const std::string& mime_type) {}

void DataOffer::Receive(const std::string& mime_type, base::ScopedFD fd) {
  const auto it = data_.find(mime_type);
  if (it == data_.end()) {
    DLOG(ERROR) << "Unexpected mime type is requested";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&WriteFileDescriptor, std::move(fd), it->second));
}

void DataOffer::Finish() {}

void DataOffer::SetActions(const base::flat_set<DndAction>& dnd_actions,
                           DndAction preferred_action) {
  dnd_action_ = preferred_action;
  delegate_->OnAction(preferred_action);
}

void DataOffer::SetSourceActions(
    const base::flat_set<DndAction>& source_actions) {
  source_actions_ = source_actions;
  delegate_->OnSourceActions(source_actions);
}

void DataOffer::SetDropData(FileHelper* file_helper,
                            const ui::OSExchangeData& data) {
  DCHECK_EQ(0u, data_.size());

  base::string16 url_list_string;
  bool found_urls = GetUrlListFromDataFile(file_helper, data, &url_list_string);
  if (!found_urls) {
    found_urls = GetUrlListFromDataPickle(file_helper, data, &url_list_string);
  }
  if (found_urls) {
    data_.emplace(file_helper->GetMimeTypeForUriList(),
                  RefCountedString16::TakeString(std::move(url_list_string)));
  } else if (data.HasString()) {
    base::string16 string_content;
    if (data.GetString(&string_content)) {
      data_.emplace(std::string(ui::Clipboard::kMimeTypeText),
                    RefCountedString16::TakeString(std::move(string_content)));
    }
  }
  for (const auto& pair : data_) {
    delegate_->OnOffer(pair.first);
  }
}

void DataOffer::SetClipboardData(FileHelper* file_helper,
                                 const ui::Clipboard& data) {
  DCHECK_EQ(0u, data_.size());
  if (data.IsFormatAvailable(ui::Clipboard::GetPlainTextWFormatType(),
                             ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    base::string16 content;
    data.ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &content);
    std::string utf8_content = base::UTF16ToUTF8(content);
    data_.emplace(std::string(kTextMimeTypeUtf8),
                  base::RefCountedString::TakeString(&utf8_content));
  }
  for (const auto& pair : data_) {
    delegate_->OnOffer(pair.first);
  }
}

bool DataOffer::GetUrlListFromDataFile(FileHelper* file_helper,
                                       const ui::OSExchangeData& data,
                                       base::string16* url_list_string) const {
  if (!data.HasFile())
    return false;
  std::vector<ui::FileInfo> files;
  if (data.GetFilenames(&files)) {
    for (const auto& info : files) {
      GURL url;
      // TODO(hirono): Need to fill the corret app_id.
      if (file_helper->GetUrlFromPath(/* app_id */ "", info.path, &url)) {
        if (!url_list_string->empty())
          *url_list_string += base::UTF8ToUTF16(kUriListSeparator);
        *url_list_string += base::UTF8ToUTF16(url.spec());
      }
    }
  }
  return !url_list_string->empty();
}

bool DataOffer::GetUrlListFromDataPickle(
    FileHelper* file_helper,
    const ui::OSExchangeData& data,
    base::string16* url_list_string) const {
  base::Pickle pickle;
  if (!data.GetPickledData(GetFileSystemFileFormatType(), &pickle))
    return false;
  std::vector<GURL> file_urls;
  if (!ReadFileUrlsFromPickle(pickle, &file_urls))
    return false;
  for (const GURL& file_url : file_urls) {
    GURL arc_url;
    if (file_helper->GetUrlFromFileSystemUrl(
            /* app_id */ "", file_url, &arc_url)) {
      if (!url_list_string->empty())
        *url_list_string += base::UTF8ToUTF16(kUriListSeparator);
      *url_list_string += base::UTF8ToUTF16(arc_url.spec());
    }
  }
  return !url_list_string->empty();
}

}  // namespace exo
