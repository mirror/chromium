def main(request, response):
    if request.method == "OPTIONS":
        response.status = 403
        response.headers.set("Content-Type", "text/plain")

    elif request.method == "GET":
        response.status = 200
        response.headers.set("Content-Type", "text/plain")
        response.headers.set("Access-Control-Allow-Credentials", "true")
        response.headers.set("Access-Control-Allow-Origin", request.headers.get("origin"))
        response.text = "Hello"

    else:
        response.status = 400
        response.headers.set("Content-Type", "text/plain")
