def main(request, response):
    code = request.GET.first("code", None)

    if(request.method == "OPTIONS"):
        if code:
            try:
                response.status = int(code)
            except:
                pass
        response.headers.set("Access-Control-Allow-Methods", "GET")
        response.headers.set("Access-Control-Max-Age", 1)
    else:
        response.status = 200

    response.headers.set("Access-Control-Allow-Origin", request.headers.get("origin"))
    response.headers.set("Access-Control-Allow-Headers", request.headers.get("Access-Control-Allow-Headers"))
