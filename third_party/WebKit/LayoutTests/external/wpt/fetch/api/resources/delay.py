import time

def main(request, response):
    delay = float(request.GET.first('ms', 500)) / 1E3
    headers = [('cache-control', 'no-store')]
    time.sleep(delay)
    return headers
