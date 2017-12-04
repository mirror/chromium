def main(request, response):
  key = request.GET.first("key")

  # 'report-uri' directive sends a violation report as a POST request containing
  # JSON data.
  if request.method == "POST":
      request.server.stash.put(key, request.body)
      return
  if request.method == "GET":
      value = request.server.stash.take(key)
      return value

  return (404, [])
