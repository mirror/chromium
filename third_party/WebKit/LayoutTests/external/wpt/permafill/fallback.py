def main(request, response):
    return [("Access-Control-Allow-Origin", "*"), ("Content-Type", "text/javascript")], 'window.log.push("fallback"); window.log.push(import.meta.url);'
