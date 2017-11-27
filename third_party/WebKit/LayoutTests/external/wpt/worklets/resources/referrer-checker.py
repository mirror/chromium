# Returns a valid response when request's |referrer| matches |referrer_policy|.
def main(request, response):
    referrer = request.headers.get("referer", "")
    expected_referrer = request.GET.first("expected_referrer")

    response_headers = [("Content-Type", "text/javascript"),
                        ("Access-Control-Allow-Origin", "*")];

    # 'no-referrer' and 'origin' cases.
    if referrer == expected_referrer:
        return (200, response_headers, "")

    # The expected referrer doesn't contain query params for simplification, so
    # we check the referrer by startswith() here.
    if referrer.startswith(expected_referrer + "?"):
        return (200, response_headers, "")

    return (404, response_headers)
