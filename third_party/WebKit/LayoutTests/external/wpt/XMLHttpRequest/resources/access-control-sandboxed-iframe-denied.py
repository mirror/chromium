def main(request, response):
    response.headers.set("Cache-Control", "no-cache")
    response.headers.set("Content-Type", "text/plain")

    # Cross-origin access should not be allowed
    response.content = "FAIL: Sandboxe iframe XHR access allowed."
