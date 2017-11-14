// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */

ProfilerTestRunner.startProfilerTest = function(callback, panel) {
  ProfilerTestRunner._panel = panel || UI.panels.js_profiler;
  TestRunner.addResult('Profiler was enabled.');
  TestRunner.addSniffer(ProfilerTestRunner._panel, '_addProfileHeader', ProfilerTestRunner._profileHeaderAdded, true);
  TestRunner.addSniffer(Profiler.ProfileView.prototype, 'refresh', ProfilerTestRunner._profileViewRefresh, true);
  TestRunner.safeWrap(callback)();
};

ProfilerTestRunner.completeProfilerTest = function() {
  TestRunner.addResult('');
  TestRunner.addResult('Profiler was disabled.');
  TestRunner.completeTest();
};

ProfilerTestRunner.runProfilerTestSuite = function(testSuite, panel) {
  var testSuiteTests = testSuite.slice();

  function runner() {
    if (!testSuiteTests.length) {
      ProfilerTestRunner.completeProfilerTest();
      return;
    }

    var nextTest = testSuiteTests.shift();
    TestRunner.addResult('');
    TestRunner.addResult(
        'Running: ' +
        /function\s([^(]*)/.exec(nextTest)[1]);
    TestRunner.safeWrap(nextTest)(runner, runner);
  }

  ProfilerTestRunner.startProfilerTest(runner, panel);
};

ProfilerTestRunner.showProfileWhenAdded = function(title) {
  ProfilerTestRunner._showProfileWhenAdded = title;
};

ProfilerTestRunner._profileHeaderAdded = function(profile) {
  if (ProfilerTestRunner._showProfileWhenAdded === profile.title)
    ProfilerTestRunner._panel.showProfile(profile);
};

ProfilerTestRunner.waitUntilProfileViewIsShown = function(title, callback) {
  callback = TestRunner.safeWrap(callback);
  var profilesPanel = ProfilerTestRunner._panel;

  if (profilesPanel.visibleView && profilesPanel.visibleView.profile &&
      profilesPanel.visibleView._profileHeader.title === title)
    callback(profilesPanel.visibleView);
  else
    ProfilerTestRunner._waitUntilProfileViewIsShownCallback = {title: title, callback: callback};

};

ProfilerTestRunner._profileViewRefresh = function() {
  if (ProfilerTestRunner._waitUntilProfileViewIsShownCallback &&
      ProfilerTestRunner._waitUntilProfileViewIsShownCallback.title === this._profileHeader.title) {
    var callback = ProfilerTestRunner._waitUntilProfileViewIsShownCallback;
    delete ProfilerTestRunner._waitUntilProfileViewIsShownCallback;
    callback.callback(this);
  }
};

ProfilerTestRunner.startSamplingHeapProfiler = function() {
  Profiler.SamplingHeapProfileType.instance.startRecordingProfile();
};

ProfilerTestRunner.stopSamplingHeapProfiler = function() {
  Profiler.SamplingHeapProfileType.instance.stopRecordingProfile();
};
