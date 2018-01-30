
#define LOG_TAG "HttpSocket"

#include "httpSocket.h"

#include <errno.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <base/logging.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace arc {

namespace {

// Maximum size of any one recv chunk, from cups/cups/http.h
constexpr int HTTP_MAX_BUFFER = 2048;

void sanitize(std::vector<char>& req) {
  for (auto& c : req) {
    if (c == '\0') {
      c = '|';
    }
  }
  req.push_back('\0');
}

}  // namespace

HttpSocket::HttpSocket() : connection_fd(-1) {}

HttpSocket::~HttpSocket() {
  ::close(connection_fd);
}

bool HttpSocket::connect() {
  LOG(ERROR) << __func__;
  connection_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_fd < 0) {
    LOG(ERROR) << "Failed to open control socket: " << strerror(errno);
    return false;
  }

  const char* sock_path = "/run/cups/cups.sock";

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, strlen(sock_path));

  int ret = TEMP_FAILURE_RETRY(
      ::connect(connection_fd, (struct sockaddr*)&addr, sizeof(addr)));
  if (ret < 0) {
    LOG(ERROR) << "Failed to connect to socket: " << strerror(errno);
    return false;
  }

  return true;
}

bool HttpSocket::poll() {
  LOG(ERROR) << __func__;
  connection_fd = -1;  // TODO(luum): clean up design-wise

  struct pollfd poll_fd;
  poll_fd.fd = connection_fd;
  poll_fd.events = POLLIN;

  int ret = ::poll(&poll_fd, 1, 1 * 1000 /* timout in ms */);
  if (ret < 1) {
    return false;
  }

  connection_fd = TEMP_FAILURE_RETRY(accept(connection_fd, nullptr, nullptr));
  if (connection_fd < 0) {
    LOG(ERROR) << "Failed to accept client socket: " << strerror(errno);
    return false;
  }

  return true;
}

int FindSubArray(const std::vector<char>& arr,
                 const std::vector<char>& to_find) {
  LOG(ERROR) << __func__;
  int found_idx = -1;
  for (size_t i = 0; i < arr.size() - to_find.size() + 1; ++i) {
    bool found = true;
    for (size_t j = 0, k = i; j < to_find.size(); ++j, ++k) {
      if (arr[k] != to_find[j]) {
        found = false;
        break;
      }
    }

    if (found == true) {
      found_idx = i;
      break;
    }
  }

  return found_idx;
}

// TODO(luum): replace with relevant base find subarray find
int FindContentLength(const std::vector<char>& request) {
  LOG(ERROR) << __func__;
  const std::string content_length_str = "Content-Length";
  const std::string nums = "0123456789";

  int start = FindSubArray(
      request,
      std::vector<char>(content_length_str.begin(), content_length_str.end()));
  if (start < 0) {
    return -1;
  }

  auto content_length_start = std::find_first_of(
      request.begin() + start, request.end(), nums.begin(), nums.end());
  if (content_length_start == request.end()) {
    return -1;
  }

  std::string content_length;
  for (auto it = content_length_start; it != request.end(); it++) {
    if (std::find(nums.begin(), nums.end(), *it) != nums.end()) {
      content_length.push_back(*it);
    } else {
      break;
    }
  }

  if (content_length.empty()) {
    return -1;
  }

  LOG(ERROR) << "Found content length: " << content_length;
  return std::stoi(content_length);
}

// TODO(luum): replace with relevant base find subarray find
int FindEndOfHeaders(const std::vector<char>& request) {
  LOG(ERROR) << __func__;
  const std::string end_of_headers_str = "\r\n\r\n";

  int end_of_headers = FindSubArray(
      request,
      std::vector<char>(end_of_headers_str.begin(), end_of_headers_str.end()));
  LOG(ERROR) << "Found end of headers: " << end_of_headers;
  return end_of_headers < 0 ? end_of_headers
                            : end_of_headers + end_of_headers_str.size();
}

// bool HttpSocket::Expect100Continue(const std::vector<char>& request) const {
// const std::string expect_100_continue_str = "HTTP/1.1 100 Continue\r\n\r\n";

// if (FindSubArray(request, std::vector<char>(expect_100_continue_str.begin(),
// expect_100_continue_str.end())) < 0) {  return false;
//}

// std::vector<uint8_t> expect_approve;
// for (auto& c : expect_100_continue_str) {
// expect_approve.push_back(static_cast<uint8_t>(c));
//}

// write(expect_approve);
// return true;
//}

bool HttpSocket::ValidateAndParseHttpRequest(
    const std::vector<uint8_t>& raw_request,
    std::vector<uint8_t>* headers,
    std::vector<uint8_t>* body) const {
  LOG(ERROR) << __func__;
  std::vector<char> request;
  for (auto& b : raw_request) {
    request.push_back(static_cast<char>(b));
  }

  std::vector<char> print_request(request);
  sanitize(print_request);
  LOG(ERROR) << "Response to validate:\n" << print_request.data();

  // if (Expect100Continue(request)) {
  // return false;
  //}

  int content_length = FindContentLength(request);
  if (content_length < 0) {
    LOG(ERROR) << "Failed to parse content-length, not full request";
    return false;
  }

  int end_of_headers = FindEndOfHeaders(request);
  if (end_of_headers < 0) {
    LOG(ERROR) << "Failed to find end of headrs, not full request";
    return false;
  }

  if (raw_request.size() < end_of_headers + content_length) {
    LOG(ERROR) << "Failed to find full body, not full request";
    return false;
  }

  std::copy(raw_request.begin(), raw_request.begin() + end_of_headers,
            std::back_inserter(*headers));
  std::copy(raw_request.begin() + end_of_headers,
            raw_request.begin() + end_of_headers + content_length,
            std::back_inserter(*body));
  return true;
}

// TODO(luum): deal with 100-continue here, speed up process
bool HttpSocket::read(std::vector<uint8_t>* headers,
                      std::vector<uint8_t>* body) const {  // TODO(luum): & -> *
  if (connection_fd < 0) {
    LOG(ERROR) << "Failed to read, no current connection";
    return false;
  }

  std::vector<uint8_t> ipp_req;
  int bytes_read = 0;
  int ret = 0;

  do {
    ipp_req.resize(bytes_read +
                   HTTP_MAX_BUFFER);  // TODO(luum): set maz buffer length

    LOG(ERROR) << "Start recv call";
    ret = recv(connection_fd, ipp_req.data() + bytes_read, HTTP_MAX_BUFFER, 0);
    LOG(ERROR) << "Received " << ret << " bytes of data";
    if (ret < 0) {  // TODO(luum): consider resetting the connection here
      LOG(ERROR) << "Failed to read: " << strerror(errno);
      return false;
    }

    bytes_read += ret;

    // TODO(luum): fix edge condition, packet_sz % HTTP_MAX_BUFFER == 0
    ipp_req.resize(bytes_read);
  } while (!ValidateAndParseHttpRequest(ipp_req, headers, body));

  return true;
}

bool HttpSocket::write(const std::vector<uint8_t>& ipp_resp) const {
  if (connection_fd < 0) {
    LOG(ERROR) << "Failed to send, no current connection";
    return false;
  }

  int bytes_written = 0;
  int ret = 0;

  while (bytes_written < ipp_resp.size()) {
    ret = send(connection_fd, ipp_resp.data(), ipp_resp.size() - bytes_written,
               0);
    LOG(ERROR) << "Wrote " << ret << " bytes of data";
    if (ret < 0) {
      LOG(ERROR) << "Failed to send: "
                 << strerror(errno);  // TODO(luum): reset connection here?
      return false;
    }

    bytes_written += ret;
  }

  return true;
}

}  // namespace arc
