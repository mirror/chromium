static const char *kScript =
  "function Process(request) {"
  "  print(request.path + ':' + request.host + ':' + request.userAgent);"
  "  print(options.foo);"
  "  print(options.bar);"
  "  options.bar = 18;"
  "  print(options.bar);"
  "}";


/**
 * A simplified http request.
 */
class StringHttpRequest : public HttpRequest {
 public:
  StringHttpRequest(const string &path, const string &referrer,
      const string &host, const string &user_agent);
  virtual const string &Path() { return path_; }
  virtual const string &Referrer() { return referrer_; }
  virtual const string &Host() { return host_; }
  virtual const string &UserAgent() { return user_agent_; }
 private:
  string path_;
  string referrer_;
  string host_;
  string user_agent_;
};


StringHttpRequest::  StringHttpRequest(const string &path,
    const string &referrer, const string &host, const string &user_agent)
    : path_(path),
      referrer_(referrer),
      host_(host),
      user_agent_(user_agent) { }
