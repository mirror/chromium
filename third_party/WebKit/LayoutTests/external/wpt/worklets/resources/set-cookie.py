def main(request, response):
    name = request.GET.first("name")
    value = request.GET.first("value")

    response.headers.set("Set-Cookie", name + "=" + value)
    response.headers.set("Access-Control-Allow-Origin", "*")
    response.headers.set("Access-Control-Allow-Credentials", "true")
