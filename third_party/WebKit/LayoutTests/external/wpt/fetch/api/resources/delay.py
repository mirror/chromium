import time

def main(request, response):
    delay = float(request.GET.first('ms', 500)) / 1E3
    time.sleep(delay)
    response.headers.set('cache-control', 'no-store')
