// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <dlfcn.h>
#import "XCTest/XCTest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

int main(int argc, char* argv[]) {
  NSString* bundle_path =
      [[NSUserDefaults standardUserDefaults] valueForKey:@"BUNDLE_PATH"];
  NSString* output_path =
      [[NSUserDefaults standardUserDefaults] valueForKey:@"OUT_DIR"];

  if (!bundle_path) {
    NSLog(@"Unspecified test bundle path");
    return 1;
  }

  NSBundle* bundle = [NSBundle bundleWithPath:bundle_path];
  const char* path =
      [[bundle executablePath] cStringUsingEncoding:NSUTF8StringEncoding];
  if (dlopen(path, RTLD_LAZY) == NULL) {
    NSLog(@"Bundle not loaded");
    return 1;
  }

  XCTestSuite* suite = [XCTestSuite testSuiteForBundlePath:bundle_path];
  NSMutableDictionary* allTests = [[NSMutableDictionary alloc] init];

  // Converts test format from '-[testSuite testMethod]' to
  // 'testSuite/testMethod' to fit in iOS test_runner.
  NSCharacterSet* splitSet =
      [NSCharacterSet characterSetWithCharactersInString:@"-[]"];
  for (XCTestSuite* testCaseSuite in suite.tests) {
    NSMutableArray* testMethods = [NSMutableArray array];
    for (XCTestCase* test in testCaseSuite.tests) {
      NSString* testName = [[test.name stringByTrimmingCharactersInSet:splitSet]
          stringByReplacingOccurrencesOfString:@" "
                                    withString:@"/"];
      if (testName) {
        [testMethods addObject:testName];
      }
    }
    [allTests setObject:[testMethods copy] forKey:testCaseSuite.name];
  }

  // Converts data into json format if a output path is passed in, otherwise
  // prints all tests.
  if (output_path) {
    NSError* error;
    NSData* jsonData =
        [NSJSONSerialization dataWithJSONObject:allTests
                                        options:NSJSONWritingPrettyPrinted
                                          error:&error];
    if (error) {
      return false;
    }
    NSString* fileName = @"list_xctests_output.json";

    NSString* outputPath =
        [output_path stringByAppendingPathComponent:fileName];
    if (![jsonData writeToFile:outputPath atomically:YES]) {
      NSLog(@"Fail to write %@", outputPath);
    }
  } else {
    NSLog(@"%@", allTests);
  }

  return 0;
}
