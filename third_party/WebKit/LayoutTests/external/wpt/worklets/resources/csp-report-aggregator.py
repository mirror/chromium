def main(request, response):
  key = request.GET.first("key")

  if request.method == "POST":
      request.server.stash.put(key, "POST")
      return
  if request.method == "GET":
      value = request.server.stash.take(key)
#      assert request.server.stash.take(key) is not None
      return value

  return (404)
