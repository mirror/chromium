def CheckChangeOnUpload(input_api, output_api):
  output = []
  output.extend(
      input_api.canned_checks.CheckChangedConfigs(input_api, output_api))
  return output


def CheckChangeOnCommit(input_api, output_api):
  output = []
  output.extend(
      input_api.canned_checks.CheckChangedConfigs(input_api, output_api))
  return output
