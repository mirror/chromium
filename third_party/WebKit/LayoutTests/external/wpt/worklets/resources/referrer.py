def main(request, response):
    policy = request.GET.first("policy", None)
    referrer = request.headers.get("referer", None)
    source_origin = request.GET.first("source_origin")
    is_cross_origin = request.GET.first("cors", None)

    if policy == "no-referrer" and not referrer:
        return (200, [("Content-Type", "text/javascript")],
                "console.log('no-referrer');")

    # When the referrer policy is "origin", |referrer| should contain only the
    # origin.
    if policy == "origin" and referrer == source_origin + "/":
        return (200, [("Content-Type", "text/javascript")],
                "console.log('origin');")

    if policy == "same-origin":
        if is_cross_origin and not referrer:
            return (200, [("Content-Type", "text/javascript"),
                          ("Access-Control-Allow-Origin", source_origin)],
                    "console.log('remote-origin');")
        if not is_cross_origin and referrer:
            return (200, [("Content-Type", "text/javascript")],
                    "console.log('same-origin');")

    return (404)
