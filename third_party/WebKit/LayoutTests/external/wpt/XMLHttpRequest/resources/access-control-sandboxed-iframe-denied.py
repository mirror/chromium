def main(request, response):
    response.headers.set("Cache-Control", "no-store")
    response.headers.set("Content-Type", "text/plain")

    # Cross-origin access should not be allowed
    response.content = "FAIL: Sandboxed iframe XHR access allowed."
