def main(request, response):
    if request.method == "POST":
        response.headers.set("Access-Control-Allow-Origin", request.headers.get("origin"))
        response.content = "PASS"

    else:
        response.content = "FAIL"
