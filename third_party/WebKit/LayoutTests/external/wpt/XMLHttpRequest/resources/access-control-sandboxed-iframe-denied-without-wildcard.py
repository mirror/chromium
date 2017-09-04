def main(request, response):
    token = request.GET.first("token", None)
    if not token:
        response.status = 400
        response.content = "Error: No token was given"
    else:
        origin = request.server.stash.take(token)
        if not origin:
            origin = request.headers.get("origin")
            request.server.stash.put(token, origin)
            response.content = "Origin set."
        else:
            # Cross-origin access should not be allowed for sandboxed iframe
            response.content = "FAIL: Sandboxed iframe XHR access allowed."

        response.headers.set("Cache-Control", "no-cache")
        response.headers.set("Content-Type", "text/plain")
        response.headers.set("Access-Control-Allow-Credentials", "true")
        response.headers.set("Access-Control-Allow-Origin", origin)
