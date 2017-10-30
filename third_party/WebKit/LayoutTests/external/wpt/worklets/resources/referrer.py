def main(request, response):
    referrer_policy = request.GET.first("referrer_policy", None)
    referrer = request.headers.get("referer", None)
    source_origin = request.GET.first("source_origin")
    is_cross_origin = request.GET.first("is_cross_origin", None)

    if referrer_policy == "no-referrer" and not referrer:
        return (200, [("Content-Type", "text/javascript"),
                      ("Access-Control-Allow-Origin", source_origin)], "")

    # When the referrer policy is "origin", |referrer| should contain only the
    # origin.
    if referrer_policy == "origin" and referrer == source_origin + "/":
        return (200, [("Content-Type", "text/javascript"),
                      ("Access-Control-Allow-Origin", source_origin)], "")

    if referrer_policy == "same-origin":
        if is_cross_origin and not referrer:
            return (200, [("Content-Type", "text/javascript"),
                          ("Access-Control-Allow-Origin", source_origin)], "")
        if not is_cross_origin and referrer:
            return (200, [("Content-Type", "text/javascript")], "")

    return (404)
