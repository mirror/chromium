def main(request, response):
    name = request.GET.first("name", None)
    response.headers.set('Set-Cookie', name + "=value")
    return
