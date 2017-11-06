# The Chromium Test Results Format

[bit.ly](https://bit.ly/chromium-test-results-format).

Stephen Martiniss, Ned Nguyen, Dirk Pranke, and Ojan Vafai.
2017-11-06
This is a **PUBLIC DRAFT** document.

[TOC]

## Introduction

The Chromium Test Results Format (*test results* for short) is a generic file
format used to record the results of each individual test in a test run.

The test results files are used so that tools (and people) don't have to
parse the output of a test run in order to determine what happened, and so
that tools can work with the results in structured ways.

### Example

Here's a very simple example for one Python test:

    % python mojo/tools/run_mojo_python_tests.py --results results.json \
        mojom_tests.parse.ast_unittest.ASTTest.testNodeBase
    Running Python unit tests under mojo/public/tools/bindings/pylib ...
    .
    ----------------------------------------------------------------------
    Ran 1 test in 0.000s

    OK
    % cat results.json
    {
      "version": 5,
      "result": "Success",
      "returncode": 0,
      "num_results_by_type": {
        "Fail": 0,
        "Pass": 1
      },
      "seconds_since_epoch": 1406662283,
      "test_delimiter": ".",
      "tests": {
        "mojom_tests": {
          "parse": {
            "ast_unittest": {
              "ASTTest": {
                "testNodeBase": {
                  "actual": [ "Pass" ],
                  "times": [ 0.1 ]
                }
              }
            }
          }
        }
      }
    }



The format consists of a one top level object containing a few fields 
summarizing the overall result, plus a set of optional metadata
fields describing the test run, plus a single `tests` field that
contains the results of every test run, structured in a hierarchical trie
format to reduce duplication of test suite names (as you can see from
the deeply hierarchical Python test name).

The file is strictly JSON-compliant. As a part of this, the order the name
appear in each object is unimportant, but the order listed above (and
in the tables below) is perhaps the most readable.

## Top-level field names

| Field Name                    | Type   | Description |
|-------------------------------|--------|-------------|
| `version`                     | number | **Required.** Version of the file format. The current version is 5. |
| `run_result`                  | string | **Required.** The result of the test run. See below for a list of the valid results. |
| `run_returncode`              | number | **Required.** The return/exit code from the test process. See below for a list of the valid results. |
| `num_results_by_type`         | dict   | **Required.** A summary of the totals of each result type. If a test was run more than once, only the first invocation's result is included in the totals. Each key is one of the result types listed below. A missing result type is the same as being present and set to zero (0). |
| `metadata`                    | dict   | **Optional.** If present, this is a dictionary of arbitrary key-value pairs of metadata fields. Key names must be strings; see below for a list of common key names. |
| `seconds_since_epoch`         | float  | **Required.** The start time of the test run expressed as a floating-point offset in seconds from the UNIX epoch. |
| `artifact_permanent_location` | string | **Optional.** A URL pointing to the permanent location of these results. |
| `artifact_type_info`          | dict   | **Optional.** The kinds of artifacts the test run might produce. See below for details. |
| `test_delimiter`              | string | **Required.** The separator string to use in between components of a tests name. |
| `tests`                       | dict   | **Required.** The actual trie of test results. Each directory or module component in the test name is a node in the trie, and the leaf contains the dict of per-test fields as described below. |
| `expectation_lists`           | list   | **Optional.** A list of src-relative paths to test list files describing the expected results of the tests. |

### Run result and returncode values

| Name                          | Value  | Description |
|-------------------------------|--------|-------------|
| `Success`                     | 0      | The run completed and all tests ran as expected. |
| `Failure`                     | 1      | The run completed and one or more tests did not run as expected. |
| `Usage`                       | 2      | The runner was passed invalid command line arguments. |
| `EarlyExit`                   | 251    | The test run was either interrupted or terminated early; the results are at best incomplete and possibly totally invalid. |
| `SysDeps`                     | 252    | Something is wrong on the machine and the test runner couldn't run (i.e., the system didn't contain the right dependencies). |
| `NoTests`                     | 253    | No tests were actually run. |
| `NoDevices`                   | 254    | No devices were present to run the tests. |
| `Unexpected`                  | 255    | Something else happened to cause the run to fail. |


### Common metadata names and values

| Field Name                    | Description |
|-------------------------------|-------------|
| `build_number`                | If this test run was produced on a builder, this should be the build number of the run, e.g., "1234". |
| `builder_name`                | If this test run was produced on a builder, this should be the builder name of the bot, e.g., "Linux Tests". |
| `master_name`                 | If this test run was produced on a builder, this should be the master name of the bot, e.g., "chromium.linux". |
| `chromium_revision`           | The commit position of the current Chromium checkout, if relevant, e.g. "356123". |
| `revision`                    | The revision of the primary repository |
| `repository`                  | A URL pointing to the location of the primary repository |

#### Artifacts

Each test may produce one or more kinds of artifacts as results; the list
of expected kinds of results across all test types is given in the top-level
`artifact_type_info` field; each individual test then contains a dict
listing each artifact name and a path to the file (relative to the top
of the results tree or the `artifact_permanent_location`, which
is also usually relative to the location of the results file, which by
convention is a file called 'results.json' in the top directory of the tree.

## Per-test fields

Each leaf of the `tests` trie contains a dict containing the results of a
particular test name. If a test is run multiple times, the dict contains the
results for each invocation in the `actual` field.

|  Field Name                   | Type     | Description |
|-------------------------------|----------|-------------|
| `actual`                      | string[] | **Required.** A list of the results the test actually produced; `["Fail", "Pass"]` means that a test was run twice, failed the first time, and then passed when it was retried. If a test produces multiple different results, then it was actually flaky during the run. |
| `times`                       | float[]  | **Required.** he times in seconds of each invocation of the test. |
| `expected`                    | string[] | **Optional.** A list of the result types expected for the test, e.g. `["Fail", "Pass"]` means that a test is expected to either pass or fail. A test that contains multiple values is expected to be flaky. If `expected` is missing, the test is expected to pass (i.e., the default value is `["Pass"]`. |
| `bugs`                        | string[] | **Optional.** A comma-separated list of URLs to bug database entries associated with each test. |
| `is_unexpected`               | bool     | **Optional.** If present and true, the failure was unexpected (a regression). If false (or if the key is not present at all), the failure was expected and will be ignored. |
| `artifacts`                   | list     | **Optional.** A list of dicts describing the types of artifacts produced by the test (see below). |

## Test result types

Any test may fail in one of several different ways. There are a few generic
types of failures, and the layout tests contain a few additional specialized
failure types.

|  Result type                  | Description |
|-------------------------------|-------------|
|  `Skip`                       | The test was not run. |
|  `Pass`                       | The test ran as expected. |
|  `Fail`                       | The test ran to completion but did not produce the expected result. |
|  `Crash`                      | The test runner crashed during the test. |
|  `Timeout`                    | The test hung (did not complete) and was aborted. |
