
import time

def main(request, response):
  time.sleep(1)

  headers = [("Content-Type", "text/javascript")]
  body = "window.didExecute = 1234;"

  return (404, "Not Found"), headers, body
