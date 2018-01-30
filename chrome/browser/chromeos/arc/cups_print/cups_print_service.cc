// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/cups_print/cups_print_service.h"

#include <utility>
#include "httpSocket.h"

#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_restrictions.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "mojo/edk/embedder/embedder.h"
#include "net/base/filename_util.h"
#include "net/http/http_util.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "url/gurl.h"

namespace {

arc::HttpSocket sock;  // TODO(luum): hacky fix; deal with httpsocket design

base::Optional<base::FilePath> SavePdf(base::File file) {
  base::AssertBlockingAllowed();

  base::FilePath file_path;
  base::CreateTemporaryFile(&file_path);
  base::File out(file_path,
                 base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);

  char buf[8192];
  ssize_t bytes;
  while ((bytes = file.ReadAtCurrentPos(buf, sizeof(buf))) > 0) {
    int written = out.WriteAtCurrentPos(buf, bytes);
    if (written < 0) {
      LOG(ERROR) << "Error while saving PDF to a disk";
      return base::nullopt;
    }
  }

  return file_path;
}

bool ValidateIppRequest(const std::vector<char>& raw_req) {
  int end_of_headers =
      net::HttpUtil::LocateEndOfHeaders(raw_req.data(), raw_req.size());
  if (end_of_headers < 0) {
    LOG(ERROR) << "End of IPP headers not found";
    return false;
  }

  // TODO(luum): failure checks?
  std::string ipp_req =
      net::HttpUtil::AssembleRawHeaders(raw_req.data(), raw_req.size());

  net::HttpUtil::HeadersIterator headers(ipp_req.begin(), ipp_req.end(),
                                         std::string(1, '\0'));
  LOG(ERROR) << "Headers:";
  while (headers.GetNext()) {
    LOG(ERROR) << headers.name() << " || " << headers.values();
  }

  return true;
}

bool ProxyToCupsd(bool new_conn,
                  const std::vector<uint8_t>& req,
                  std::vector<uint8_t>* resp) {
  LOG(ERROR) << __func__;
  if (new_conn) {
    if (!sock.connect()) {
      LOG(ERROR) << "Failed to connect to cups";
      return false;
    }
  }

  // Send req
  sock.write(req);
  // int bytes_sent = 0;
  // while (bytes_sent < req.size()) { // TODO(luum): send in HTTP_MAX_BUFFER
  // chunks?  ret = send(fd, req.data() + bytes_sent, req.size() - bytes_sent, 0);
  // if (ret < 0) {
  // LOG(ERROR) << "Failed to send: " << strerror(errno);
  // return false;
  //}

  // bytes_sent += ret;
  //}

  std::vector<uint8_t> headers;
  std::vector<uint8_t> body;
  sock.read(&headers, &body);
  // Wait for resp
  // int bytes_read = 0, count = 0;
  // do {
  // resp->resize(bytes_read + HTTP_MAX_BUFFER);

  // ret = recv(fd, resp->data() + bytes_read, HTTP_MAX_BUFFER, 0);
  // if (ret < 0) {
  // LOG(ERROR) << "Failed read: " << strerror(errno);
  // return false;
  //}

  // bytes_read += ret;
  // count++;
  //} while(count < 5);

  // resp->resize(bytes_read);
  std::copy(headers.begin(), headers.end(), std::back_inserter(*resp));
  std::copy(body.begin(), body.end(), std::back_inserter(*resp));
  return true;
}

}  // namespace

namespace arc {
namespace {

// Singleton factory for ArcCupsPrintService.
class ArcCupsPrintServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcCupsPrintService,
          ArcCupsPrintServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcCupsPrintServiceFactory";

  static ArcCupsPrintServiceFactory* GetInstance() {
    return base::Singleton<ArcCupsPrintServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcCupsPrintServiceFactory>;
  ArcCupsPrintServiceFactory() = default;
  ~ArcCupsPrintServiceFactory() override = default;
};

}  // namespace

// static
ArcCupsPrintService* ArcCupsPrintService::GetForBrowserContext(
    content::BrowserContext* context) {
  LOG(ERROR) << __func__;
  return ArcCupsPrintServiceFactory::GetForBrowserContext(context);
}

ArcCupsPrintService::ArcCupsPrintService(content::BrowserContext* context,
                                         ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service), weak_ptr_factory_(this) {
  LOG(ERROR) << __func__;
  arc_bridge_service_->cups_print()->AddObserver(this);
  arc_bridge_service_->cups_print()->SetHost(this);
}

ArcCupsPrintService::~ArcCupsPrintService() {
  LOG(ERROR) << __func__;
  arc_bridge_service_->cups_print()->RemoveObserver(nullptr);
  arc_bridge_service_->cups_print()->SetHost(nullptr);
}

void ArcCupsPrintService::OnConnectionReady() {
  LOG(ERROR) << __func__;
  // mojom::CupsPrintInstance* cups_print_instance =
  // ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->cups_print(), Init);
}

void sanitize(std::vector<char>& message) {
  for (auto& c : message)
    if (c == '\0')
      c = '|';

  message.push_back('\0');
}

void ArcCupsPrintService::SendIppRequest(bool new_connection,
                                         const std::vector<uint8_t>& headers,
                                         const std::vector<uint8_t>& body,
                                         SendIppRequestCallback callback) {
  std::vector<uint8_t> ipp_req(headers);
  ipp_req.insert(ipp_req.end(), body.begin(), body.end());

  LOG(ERROR) << __func__;
  LOG(ERROR) << "IPP request recvd, size: " << ipp_req.size();
  LOG(ERROR) << "New connection status: " << new_connection;

  std::vector<char> req;
  for (auto& b : ipp_req)
    req.push_back(static_cast<char>(b));
  sanitize(req);
  LOG(ERROR) << "Request reads:\n" << req.data();

  // TODO(luum): should I avoid copying/what can I do with unvalidated data
  // if (!ValidateIppRequest(req)) {
  // LOG(ERROR) << "IPP request failed validation";
  // std::move(callback).Run(mojom::CupsPrintResult::FAILURE, base::nullopt);
  //}

  std::vector<uint8_t> ipp_resp;
  if (!ProxyToCupsd(new_connection, ipp_req, &ipp_resp)) {
    LOG(ERROR) << "Failed to proxy to cupsd";
    std::move(callback).Run(mojom::CupsPrintResult::FAILURE, base::nullopt);
  }

  LOG(ERROR) << "IPP response recvd, size: " << ipp_resp.size();

  std::vector<char> resp;
  for (auto& b : ipp_resp)
    resp.push_back(static_cast<char>(b));
  sanitize(resp);
  LOG(ERROR) << "Response reads:\n" << resp.data();

  std::move(callback).Run(mojom::CupsPrintResult::SUCCESS, std::move(ipp_resp));
}

void ArcCupsPrintService::Ask(const std::string& message, AskCallback cb) {
  LOG(ERROR) << __func__;
  LOG(ERROR) << "Received message: " << message;

  std::string reply = "yup";
  LOG(ERROR) << "Returning: " << reply;
  std::move(cb).Run(mojom::CupsPrintResult::SUCCESS, reply);
}

/*void ArcCupsPrintService::Print(mojo::ScopedHandle pdf_data) {*/
// if (!pdf_data.is_valid()) {
// LOG(ERROR) << "handle is invalid";
// return;
//}

// mojo::edk::ScopedPlatformHandle scoped_platform_handle;
// MojoResult mojo_result = mojo::edk::PassWrappedPlatformHandle(
// pdf_data.release().value(), &scoped_platform_handle);
// if (mojo_result != MOJO_RESULT_OK) {
// LOG(ERROR) << "PassWrappedPlatformHandle failed: " << mojo_result;
// return;
//}

// base::File file(scoped_platform_handle.release().handle);

// base::PostTaskWithTraitsAndReplyWithResult(
// FROM_HERE, {base::MayBlock()},
// base::BindOnce(&SavePdf, base::Passed(&file)),
// base::BindOnce(&ArcCupsPrintService::OpenPdf,
// weak_ptr_factory_.GetWeakPtr()));
/*}*/

void ArcCupsPrintService::OpenPdf(
    base::Optional<base::FilePath> file_path) const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!file_path)
    return;

  GURL gurl = net::FilePathToFileURL(file_path.value());
  ash::Shell::Get()->shell_delegate()->OpenUrlFromArc(gurl);
  // TODO(poromov) Delete file after printing. (http://crbug.com/629843)
}

}  // namespace arc
