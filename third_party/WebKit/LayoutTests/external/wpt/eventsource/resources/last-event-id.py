def main(request, response):
  response.headers.set("Content-Type", "text/event-stream")

  last_event_id = request.headers.get('Last-Event-ID', None)
  if last_event_id:
    return "data: " + last_event_id + "\n\n"
  else:
    idvalue = request.GET.first("idvalue", "\xe2\x80\xa6")
    return "id: " + idvalue + "\nretry: 200\ndata: hello\n\n"
