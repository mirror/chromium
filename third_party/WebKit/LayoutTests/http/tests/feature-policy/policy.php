<?php
// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test ensures that fullscreen feature when enabled for self only works
// in the same orgin but not cross origins when allowfullscreen is set. No
// iframe may call it when allowfullscreen is not set.

Header("Feature-Policy: fullscreen *; payment 'self'; midi 'none'; camera 'self' https://www.example.com https://www.example.net");
?>

<!DOCTYPE html>
<script src="../../resources/testharness.js"></script>
<script src="../../resources/testharnessreport.js"></script>
<iframe></iframe>
<script>
var policy_main = document.policy;
var policy_iframe = document.querySelector('iframe').policy;
var allowed_features = ["fullscreen", "payment", "camera"];
var disallowed_features = ["badfeature", "midi"];

// Tests for policy.allowsFeature().
for (var i = 0; i < allowed_features.length; i++) {
  var feature = allowed_features[i];
  test(function() {
    assert_true(policy_main.allowsFeature(feature));
    assert_true(policy_main.allowsFeature(feature, "http://127.0.0.1:8000"));
    assert_true(policy_iframe.allowsFeature(feature));
    assert_true(policy_iframe.allowsFeature(feature, "http://127.0.0.1:8000"));
  }, 'Test policy.allowsFeature() on feature ' + feature);
}
test(function() {
  assert_true(policy_main.allowsFeature("camera", "https://www.example.com"));
  assert_true(policy_main.allowsFeature("camera", "https://www.example.net/"));
  assert_true(policy_iframe.allowsFeature("camera", "https://www.example.com"));
  assert_true(policy_iframe.allowsFeature("camera", "https://www.example.net/"));
}, 'Test policy.allowsFeature() for camera');

for (var i = 0; i < disallowed_features.length; i++) {
  var feature = disallowed_features[i];
  test(function() {
    assert_false(policy_main.allowsFeature(feature));
    assert_false(policy_main.allowsFeature(feature, "http://127.0.0.1:8000"));
    assert_false(policy_iframe.allowsFeature(feature));
    assert_false(policy_iframe.allowsFeature(feature, "http://127.0.0.1:8000"));
  }, 'Test policy.allowsFeature() on disallowed feature ' + feature);
}

// Tests for policy.allowedFeatures().
var allowed_features_main = policy_main.allowedFeatures();
var allowed_features_iframe = policy_iframe.allowedFeatures();
for (var i = 0; i < allowed_features.length; i++) {
  var feature = allowed_features[i];
  test(function() {
    assert_true(allowed_features_main.includes(feature));
    assert_true(allowed_features_iframe.includes(feature));
  }, 'Test policy.allowedFeatures() include feature ' + feature);
}
for (var i = 0; i < disallowed_features.length; i++) {
  var feature = disallowed_features[i];
  test(function() {
    assert_false(allowed_features_main.includes(feature));
    assert_false(allowed_features_iframe.includes(feature));
  }, 'Test policy.allowedFeatures() does not include disallowed feature ' +
    feature);
}

// Tests for policy.getAllowlist().
assert_array_equals(
  policy_main.getAllowlist("fullscreen"), ["*"],
  "fullscreen is allowed for all in main frame");
assert_array_equals(
  policy_iframe.getAllowlist("fullscreen"), ["*"],
  "fullscreen is allowed for all in the iframe");
assert_array_equals(
  policy_main.getAllowlist("payment"), ["http://127.0.0.1:8000"],
  "payment is allowed for self");
assert_array_equals(
  policy_iframe.getAllowlist("payment"), ["http://127.0.0.1:8000"],
  "payment is allowed for self");
assert_array_equals(
  policy_main.getAllowlist("camera"),
  ["http://127.0.0.1:8000", "https://www.example.com", "https://www.example.net"],
  "camera is allowed for multiple origins");
assert_array_equals(
  policy_iframe.getAllowlist("camera"),
  ["http://127.0.0.1:8000", "https://www.example.com", "https://www.example.net"],
  "camera is allowed for multiple origins");
assert_array_equals(
  policy_main.getAllowlist("midi"), [],
  "midi is disallowed for all");
assert_array_equals(
  policy_iframe.getAllowlist("midi"), [],
  "midi is disallowed for all");
</script>
