
#ifndef CUPS_PRINT_HTTPSOCKET_H_
#define CUPS_PRINT_HTTPSOCKET_H_

#include <vector>

namespace arc {

class HttpSocket {
 public:
  explicit HttpSocket();
  ~HttpSocket();

  bool connect();
  bool poll();

  bool read(std::vector<uint8_t>* headers, std::vector<uint8_t>* body) const;
  bool write(const std::vector<uint8_t>& ipp_resp) const;

 private:
  // bool Expect100Continue(const std::vector<char>& request) const;
  bool ValidateAndParseHttpRequest(const std::vector<uint8_t>& raw_request,
                                   std::vector<uint8_t>* headers,
                                   std::vector<uint8_t>* body) const;

  int sock;
  int connection_fd;
};

}  // namespace arc

#endif  // CUPS_PRINT_HTTPSOCKET_H_
