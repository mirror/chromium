def main(request, response):
    response.headers.set("Cache-Control", "no-store")
    response.headers.set("Access-Control-Allow-Origin", "*")
    response.headers.set("Access-Control-Max-Age", 0)

    if request.method == "OPTIONS":
        if request.headers.get("x-test") != "":
            response.headers.set("Access-Control-Allow-Headers", "X-Test")
    elif request.method == "GET":
        if request.headers.get("x-test") != "":
            response.content = "PASS"
        else:
            response.status = 400
