// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_FILTER_MOCK_FILTER_CONTEXT_H_
#define NET_FILTER_MOCK_FILTER_CONTEXT_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "net/filter/filter.h"
#include "url/gurl.h"

namespace net {

class URLRequestContext;

class MockFilterContext : public FilterContext {
 public:
  MockFilterContext();
  virtual ~MockFilterContext();

  void SetMimeType(const std::string& mime_type) { mime_type_ = mime_type; }
  void SetURL(const GURL& gurl) { gurl_ = gurl; }
  void SetContentDisposition(const std::string& disposition) {
    content_disposition_ = disposition;
  }
  void SetRequestTime(const base::Time time) { request_time_ = time; }
  void SetCached(bool is_cached) { is_cached_content_ = is_cached; }
  void SetDownload(bool is_download) { is_download_ = is_download; }
  void SetResponseCode(int response_code) { response_code_ = response_code; }
  void SetSdchResponse(bool is_sdch_response) {
    is_sdch_response_ = is_sdch_response;
  }
  URLRequestContext* GetModifiableURLRequestContext() const {
    return context_.get();
  }

  // After a URLRequest's destructor is called, some interfaces may become
  // unstable.  This method is used to signal that state, so we can tag use
  // of those interfaces as coding errors.
  void NukeUnstableInterfaces();

  virtual bool GetMimeType(std::string* mime_type) const override;

  // What URL was used to access this data?
  // Return false if gurl is not present.
  virtual bool GetURL(GURL* gurl) const override;

  // What Content-Disposition did the server supply for this data?
  // Return false if Content-Disposition was not present.
  virtual bool GetContentDisposition(std::string* disposition) const override;

  // What was this data requested from a server?
  virtual base::Time GetRequestTime() const override;

  // Is data supplied from cache, or fresh across the net?
  virtual bool IsCachedContent() const override;

  // Is this a download?
  virtual bool IsDownload() const override;

  // Was this data flagged as a response to a request with an SDCH dictionary?
  virtual bool SdchResponseExpected() const override;

  // How many bytes were fed to filter(s) so far?
  virtual int64 GetByteReadCount() const override;

  virtual int GetResponseCode() const override;

  // The URLRequestContext associated with the request.
  virtual const URLRequestContext* GetURLRequestContext() const override;

  virtual void RecordPacketStats(StatisticSelector statistic) const override {}

 private:
  int buffer_size_;
  std::string mime_type_;
  std::string content_disposition_;
  GURL gurl_;
  base::Time request_time_;
  bool is_cached_content_;
  bool is_download_;
  bool is_sdch_response_;
  bool ok_to_call_get_url_;
  int response_code_;
  scoped_ptr<URLRequestContext> context_;

  DISALLOW_COPY_AND_ASSIGN(MockFilterContext);
};

}  // namespace net

#endif  // NET_FILTER_MOCK_FILTER_CONTEXT_H_
