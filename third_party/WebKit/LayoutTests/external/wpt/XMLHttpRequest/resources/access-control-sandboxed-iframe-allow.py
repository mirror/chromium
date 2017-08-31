def main(request, response):
    response.headers.set("Cache-Control", "no-cache")
    response.headers.set("Content-Type", "text/plain")
    response.headers.set("Access-Control-Allow-Credentials", "true")
    response.headers.set("Access-Control-Allow-External", "true")
    response.headers.set("Access-Control-Allow-Origin", "*")

    response.content = "PASS: Sandboxed iframe XHR access allowed."
