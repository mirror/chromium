def main(request, response):
    headers = {
            "Cache-Control": "no-cache",
            "Access-Control-Allow-Headers": "X-Requested-With",
            "Access-Control-Max-Age": 0,
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Methods": "*",
            "Vary": "Accept-Encoding",
            "Content-Type": "text/plain"
    }

    for header in headers:
        response.headers.set(header, headers[header])

    response.content = "PASS"
