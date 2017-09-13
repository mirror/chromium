import datetime
import time

def main(request, response):
    cookie_name = request.GET.first("cookie_name", None)
    reset_cookies = request.GET.first("reset_cookies", None)
    current_time_in_seconds = int(round(time.time()))

    response.headers.set("Cache-Control", "no-store")
    response.headers.set("Access-Control-Allow-Origin", request.headers.get("origin"))
    response.headers.set("Access-Control-Allow-Credentials", "true")

    if reset_cookies:
        for cookie in request.cookies:
            # Set cookie to expire yesterday
            response.set_cookie(cookie_name, "deleted", expires=
                                datetime.datetime.fromtimestamp(current_time_in_seconds - 24*60*60))
    if cookie_name:
        # Set cookie to expire tomorrow
        response.set_cookie(cookie_name, "COOKIE", expires=
                            datetime.datetime.fromtimestamp(current_time_in_seconds + 24*60*60))
    elif (not cookie_name) and (not reset_cookies):
        response.status = 400
