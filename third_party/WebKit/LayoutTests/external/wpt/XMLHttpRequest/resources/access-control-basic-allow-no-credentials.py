#!/usr/bin/env python
def main(request, response):
    response.headers.set("Content-Type", "text/plain")
    response.headers.set("Access-Control-Allow-Origin", "http://web-platform.test:8001")

    response.content = "PASS: Cross-domain access allowed."
