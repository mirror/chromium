// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/permafill/browser_protocol.h"

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/permafill/resources.h"
#include "content/common/permafill/constants.h"
#include "content/public/common/content_client.h"
#include "net/base/mime_util.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_redirect_job.h"
#include "net/url_request/url_request_simple_job.h"

namespace permafill {

class BrowserProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  BrowserProtocolHandler() {}

  ~BrowserProtocolHandler() override {}

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserProtocolHandler);
};

bool IsImplemented(const std::string& permafill) {
  // TODO(hiroshige): Supported permafills are hard-coded here.
  if (permafill == "test")
    return true;
  return false;
}

// Copied from chrome/browser/extensions/chrome_url_request_util.h.

// A request for an extension resource in a Chrome .pak file. These are used
// by component extensions.
class URLRequestResourceBundleJob : public net::URLRequestSimpleJob {
 public:
  URLRequestResourceBundleJob(net::URLRequest* request,
                              net::NetworkDelegate* network_delegate,
                              const base::FilePath& filename,
                              int resource_id)
      : net::URLRequestSimpleJob(request, network_delegate),
        filename_(filename),
        resource_id_(resource_id),
        weak_factory_(this) {
    std::string raw_headers;
    raw_headers.append("HTTP/1.1 200 OK");

    // 1.f.4. Append `Content-Type`/`text/javascript` to response's header
    // list.
    raw_headers.append(1, '\0');
    raw_headers.append("Content-Type: text/javascript");

    // 1.f.5. Append `Access-Control-Allow-Origin`/`*` to response's header
    // list.
    raw_headers.append(1, '\0');
    raw_headers.append("Access-Control-Allow-Origin: *");

    raw_headers.append(2, '\0');
    response_info_.headers = new net::HttpResponseHeaders(raw_headers);
  }

  // Overridden from URLRequestSimpleJob:
  int GetRefCountedData(
      std::string* mime_type,
      std::string* charset,
      scoped_refptr<base::RefCountedMemory>* data,
      const net::CompletionCallback& callback) const override {
    *data = content::GetContentClient()->GetDataResourceBytes(resource_id_);
    /*
    const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    *data = rb.LoadDataResourceBytes(resource_id_);
    */
    if (!*data)
      return net::ERR_FAILED;

    // Add the Content-Length header now that we know the resource length.
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentLength,
                           base::NumberToString((*data)->size()).c_str()));

    std::string* read_mime_type = new std::string;
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&net::GetMimeTypeFromFile, filename_,
                       base::Unretained(read_mime_type)),
        base::BindOnce(&URLRequestResourceBundleJob::OnMimeTypeRead,
                       weak_factory_.GetWeakPtr(), mime_type, charset, *data,
                       base::Owned(read_mime_type), callback));

    return net::ERR_IO_PENDING;
  }

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    *info = response_info_;
  }

 private:
  ~URLRequestResourceBundleJob() override {}

  void OnMimeTypeRead(std::string* out_mime_type,
                      std::string* charset,
                      scoped_refptr<base::RefCountedMemory> data,
                      std::string* read_mime_type,
                      const net::CompletionCallback& callback,
                      bool read_result) {
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentType,
                           read_mime_type->c_str()));
    *out_mime_type = *read_mime_type;
    if (base::StartsWith(*read_mime_type, "text/",
                         base::CompareCase::INSENSITIVE_ASCII)) {
      // All of our HTML files should be UTF-8 and for other resource types
      // (like images), charset doesn't matter.
      DCHECK(base::IsStringUTF8(base::StringPiece(
          reinterpret_cast<const char*>(data->front()), data->size())));
      *charset = "utf-8";
    }
    int result = read_result ? net::OK : net::ERR_INVALID_URL;
    callback.Run(result);
  }

  // We need the filename of the resource to determine the mime type.
  base::FilePath filename_;

  // The resource bundle id to load.
  int resource_id_;

  net::HttpResponseInfo response_info_;

  mutable base::WeakPtrFactory<URLRequestResourceBundleJob> weak_factory_;
};

// Creates URLRequestJobs for browser:// URLs.
//
// https://docs.google.com/document/d/1VbU4z7xtU_kzuLAcj38KKL5qoOr2UYNUJW8vZB2AcWc/edit#heading=h.d8xhrnoec1xa
net::URLRequestJob* BrowserProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  // 1.a. If request's method is not `GET`, then return a network error.
  if (request->method() != "GET") {
    return new net::URLRequestErrorJob(request, network_delegate,
                                       net::ERR_FAILED);
  }

  // 1.c. Let path be request's current url's path[0].
  // 1.d. Let permafill be the portion of path before the first U+007C (|),
  // or all of path if no U+007C (|) is present.
  // 1.e. Let fallback be the portion of path after the first U+007C (|),
  // or null if no U+007C (|) is present.
  //
  // TODO(hiroshige): Currently browser://<permafill-name>?<fallback-url>
  // is used instead of browser:<permafill-name>|<fallback-url>.
  const std::string permafill = request->url().host();
  const std::string fallback = request->url().query();

  // 1.f. If the browser implements the permafill identified by permafill, then:
  if (IsImplemented(permafill)) {
    if (permafill == "test") {
      // 1.f.1. Let response be a new response.

      // 1.f.2. Let responseURL be the result of parsing the concatenation
      // of "browser:" with permafill.

      // 1.f.3. Set response's url list to << responseURL >>.

      // 1.f.6. Set response's HTTPS state to request's client's HTTPS state
      // if request's client is non-null.

      // 1.f.7. Set response's body to the bytes corresponding to the p
      // ermafill script identified by permafill.

      // 1.f.8. Return response.
    }

    if (!fallback.empty()) {
      // Redirect to browser://<permafill-name> just to set the response URL
      // correctly.
      return new net::URLRequestRedirectJob(
          request, network_delegate,
          GURL(std::string(kBrowserScheme) + "://" + permafill),
          net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
          "Permafill");
    }

    const int resource_id = GetResourceIDFromPath(permafill + "/index.js");
    DCHECK_GE(resource_id, 0);

    return new URLRequestResourceBundleJob(
        request, network_delegate, base::FilePath("index.js"), resource_id);

    /*
    // TODO(hiroshige): Currently implemented as a redirect to
    // chrome-extension.
    return new net::URLRequestRedirectJob(
        request, network_delegate,
        GURL("chrome-extension://acihfchgdiicfpkconamoakocligclao/" +
             permafill + "/index.js"),
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Permafill");
    */
  }

  // 1.g. Otherwise, if fallback is non-null, then:
  if (!fallback.empty()) {
    // 1.g.1. Let fallbackURL be the result of parsing fallback. If this
    // fails, then return a network error.
    GURL fallback_url(fallback);
    if (!fallback_url.is_valid()) {
      return new net::URLRequestErrorJob(request, network_delegate,
                                         net::ERR_FAILED);
    }

    // 1.g.2. Let response be a new response.
    // 1.g.3. Set response's status to 307.
    // 1.g.4. Let fallbackURLBytes be fallbackURL, serialized and UTF-8
    // encoded. 1.g.5. Append `Location`/fallbackURL to response's header
    // list. 1.g.6. Return response.
    return new net::URLRequestRedirectJob(
        request, network_delegate, fallback_url,
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Fallback");
  }

  // 1.h. Otherwise, the browser does not implement the given permafill and no
  // fallback was provided; return a network error.
  return new net::URLRequestErrorJob(request, network_delegate,
                                     net::ERR_FAILED);
}

std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateBrowserProtocolHandler() {
  return std::make_unique<BrowserProtocolHandler>();
}

}  // namespace permafill
