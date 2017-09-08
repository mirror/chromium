def main(request, response):
    headers = {
            "Cache-Control": "no-cache",
            "Access-Control-Max-Age": 0,
            "Access-Control-Allow-Origin": "*",
            "X-foo": "BAR",
            "x-test": "TEST",
            "Access-Control-Expose-Headers": "x-Foo",
            "Content-Type": "text/html"
    }

    for header in headers:
        response.headers.set(header, headers[header])

    response.content = "PASS"
