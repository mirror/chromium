def main(request, response):
    policy = request.GET.first("policy", None)
    origin = request.headers.get("origin")

    if policy == "no-referrer" and request.headers.get("referer") == None:
        return (200, [("Content-Type", "text/javascript")],
                "console.log('no-referrer');")

    if policy == "origin" and request.headers.get("referer") == origin + "/":
        return (200, [("Content-Type", "text/javascript")],
                "console.log('origin');")

    return (404)
