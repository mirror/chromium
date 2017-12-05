from os import path;

SETUP_FILE_TEMPLATE = "{}-test"
EXPECTATION_FILE_TEMPLATE = "{}-expected"
DEFAULT_RESOURCE_DIR = path.join("cookies", "http-state", "resources")

def dump_file_contents(directory, filename):
  return open(path.join(directory, filename), "r").read()

def main(request, response):
  if not "file" in request.GET:
    return;

  test_file = SETUP_FILE_TEMPLATE.format(request.GET['file']);
  expectation_file = EXPECTATION_FILE_TEMPLATE.format(request.GET['file']);
  res_dir = path.join(request.doc_root, DEFAULT_RESOURCE_DIR)

  response.writer.write_status(200)
  response.writer.write(dump_file_contents(res_dir, test_file))
  response.writer.end_headers()
  response.writer.write_content(dump_file_contents(res_dir, expectation_file))
  response.writer.write()
