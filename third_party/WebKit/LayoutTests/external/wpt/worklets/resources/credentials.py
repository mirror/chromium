def main(request, response):
    mode = request.GET.first("mode", None)
    cookie = request.cookies.first("cookieName", None)

    # Reply a 404 response when the following conditions are not satisfied to
    # fail addModule().

    # The request with the default WorkletOptions or "credentials=omit" should
    # not include the cookie.
    if (mode == None or mode == "omit") and cookie != None:
        return (404)

    # The request with "credentials=include" should include the cookie.
    if mode == "include" and cookie == None:
        return (404)

    return (200, [("Content-Type", "text/javascript")], "Success")
