def main(request, response):
    response.headers.set("Cache-Control", "no-store")
    response.headers.set("Access-Control-Allow-Origin", "*")

    if request.method == "OPTIONS":
        if not "origin" in request.headers.get("Access-Control-Request-Headers"):
            response.headers.set("Access-Control-Allow-Headers", "x-pass")
    else:
        response.content = request.headers.get("x-pass")
