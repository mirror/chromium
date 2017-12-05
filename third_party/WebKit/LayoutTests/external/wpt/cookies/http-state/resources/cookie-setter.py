from os import path;

import Cookie

def main(request, response):
  if "file" in request.GET:
    test_file = "%s-test" % request.GET['file'];
    response_data = open(path.join(request.doc_root, "cookies",
                         "http-state", "resources", test_file)).read()

    response.writer.write_status(200)
    response.writer.write(response_data)

    if not response.writer._response.explicit_flush:
      response.writer.flush()

    response.writer.end_headers()
    response.writer.write_content(open(path.join(request.doc_root, "cookies",
                         "http-state", "resources", "%s-expected" % request.GET['file'])).read())
    response.writer.write()

  if "clean" in request.GET:
    credentials = request.GET.first("credentials", "true")

    headers = [("Content-Type", "text/plain")]
    if credentials != 'none':
        headers.append(("Access-Control-Allow-Credentials", credentials))

    body = "Deleted cookies: "
    parser = Cookie.BaseCookie()
    parser.load(request.headers.get("cookie"))
    for key, _ in parser.iteritems():
      if key in request.cookies:
        body += "{}:{}; ".format(key, request.cookies[key].value)
        response.delete_cookie(key)

    return headers, body
    # response.writer.write_status(200)
    # parser = Cookie.BaseCookie()
    # parser.load(request.headers.get("cookie"))
    # for key, _ in parser.iteritems():
    #   response.delete_cookie(key)
    # response.writer.end_headers()


    # response.writer.write_content(request.headers.get("cookie"))
    # response.writer.write()
