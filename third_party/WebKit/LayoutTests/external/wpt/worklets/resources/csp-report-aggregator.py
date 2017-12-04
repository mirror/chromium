def main(request, response):
  key = request.GET.first("key")

  if request.method == "POST":
      request.server.stash.put(key, request.body)
      return
  if request.method == "GET":
      value = request.server.stash.take(key)
#      assert request.server.stash.take(key) is not None
      return value

  return (404, [])
