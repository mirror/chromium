import time

def main(request, response):
    content = 'TEST_TRICKLE\n'
    delay = float(request.GET.first('ms', 500)) / 1E3
    count = int(request.GET.first('count', 50))
    time.sleep(delay)
    response.headers.set('content-type', "text/plain")
    response.headers.set('content-length', count * len(content))
    response.write_status_headers()
    time.sleep(delay);
    for i in xrange(count):
        response.writer.write_content(content)
        time.sleep(delay)
