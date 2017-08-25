def main(request, response):
    # CORS-safelisted
    headers = {
            "content-type": "text/plain",
            "cache-control": "no cache",
            "content-language": "en",
            "expires": "Fri, 30 Oct 1998 14:19:41 GMT",
            "last-modified": "Tue, 15 Nov 1994 12:45:26 GMT",
            "pragma": "no-cache"
    }
    for header in headers:
        response.headers.set(header, headers[header])

    # Non-CORS-safelisted
    response.headers.set("x-test", "foobar")

    response.headers.set("Access-Control-Allow-Origin", "*")
    response.content = "PASS: Cross-domain access allowed."
