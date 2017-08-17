def main(request, response):
    response.headers.set("Cache-Control", "no-store")

    # This should be a simple request; deny preflight
    if(request.method == "POST"):
        response.headers.set("Access-Control-Allow-Credentials", "true")
        response.headers.set("Access-Control-Allow-Origin", request.headers.get("origin"))

        response.content = "Accept: " + request.headers.get("Accept") + "\n"
        response.content += "Accept-Language: " + request.headers.get("Accept-Language") + "\n"
        response.content += "Content-Language: " + request.headers.get("Content-Language") + "\n"
        response.content += "Content-Type: " + request.headers.get("Content-Type")
