# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from gpu_tests import gpu_integration_test
from gpu_tests import gpu_process_expectations
from gpu_tests import path_util

data_path = os.path.join(
    path_util.GetChromiumSrcDir(), 'content', 'test', 'data')

test_harness_script = r"""
  var domAutomationController = {};
  domAutomationController._finished = false;
  domAutomationController.send = function(msg) {
    domAutomationController._finished = true;
  }

  window.domAutomationController = domAutomationController;
"""

class GpuProcessIntegrationTest(gpu_integration_test.GpuIntegrationTest):
  @classmethod
  def Name(cls):
    """The name by which this test is invoked on the command line."""
    return 'gpu_process'

  @classmethod
  def SetUpProcess(cls):
    super(GpuProcessIntegrationTest, cls).SetUpProcess()
    cls.CustomizeBrowserArgs(cls._AddDefaultArgs([]))
    cls.StartBrowser()
    cls.SetStaticServerDirs([data_path])

  @staticmethod
  def _AddDefaultArgs(browser_args):
    # All tests receive the following options.
    return [
      '--enable-gpu-benchmarking',
      # TODO(kbr): figure out why the following option seems to be
      # needed on Android for robustness.
      # https://github.com/catapult-project/catapult/issues/3122
      '--no-first-run'] + browser_args

  @classmethod
  def _CreateExpectations(cls):
    return gpu_process_expectations.GpuProcessExpectations()

  @classmethod
  def GenerateGpuTests(cls, options):
    # The browser test runner synthesizes methods with the exact name
    # given in GenerateGpuTests, so in order to hand-write our tests but
    # also go through the _RunGpuTest trampoline, the test needs to be
    # slightly differently named.

    # Also note that since functional_video.html refers to files in
    # ../media/ , the serving dir must be the common parent directory.
    tests = (('GpuProcess_canvas2d', 'gpu/functional_canvas_demo.html'),
             ('GpuProcess_css3d', 'gpu/functional_3d_css.html'),
             ('GpuProcess_webgl', 'gpu/functional_webgl.html'),
             ('GpuProcess_video', 'gpu/functional_video.html'),
             ('GpuProcess_gpu_info_complete', 'gpu/functional_3d_css.html'),
             ('GpuProcess_no_gpu_process', 'about:blank'),
             ('GpuProcess_readback_webgl_gpu_process', 'chrome:gpu'),
             ('GpuProcess_only_one_workaround', 'chrome:gpu'),
             ('GpuProcess_skip_gpu_process', 'gpu/functional_webgl.html'),
             ('GpuProcess_identify_active_gpu1', 'chrome:gpu'),
             ('GpuProcess_identify_active_gpu2', 'chrome:gpu'),
             ('GpuProcess_identify_active_gpu3', 'chrome:gpu'),
             ('GpuProcess_identify_active_gpu4', 'chrome:gpu'),
             ('GpuProcess_swiftshader_for_webgl', 'gpu/functional_webgl.html'))

    # The earlier has_transparent_visuals_gpu_process and
    # no_transparent_visuals_gpu_process tests became no-ops in
    # http://crrev.com/2347383002 and were deleted.

    for t in tests:
      yield (t[0], t[1], ('_' + t[0]))

  def RunActualGpuTest(self, test_path, *args):
    test_name = args[0]
    getattr(self, test_name)(test_path)

  ######################################
  # Helper functions for the tests below

  def _Navigate(self, test_path):
    url = self.UrlOfStaticFilePath(test_path)
    # It's crucial to use the action_runner, rather than the tab's
    # Navigate method directly. It waits for the document ready state
    # to become interactive or better, avoiding critical race
    # conditions.
    self.tab.action_runner.Navigate(
      url, script_to_evaluate_on_commit=test_harness_script)

  def _NavigateAndWait(self, test_path):
    self._Navigate(test_path)
    self.tab.action_runner.WaitForJavaScriptCondition(
      'window.domAutomationController._finished', timeout=10)

  def _VerifyGpuProcessPresent(self):
    tab = self.tab
    if not tab.EvaluateJavaScript('chrome.gpuBenchmarking.hasGpuChannel()'):
      self.fail('No GPU channel detected')

  # This can only be called from one of the tests, i.e., after the
  # browser's been brought up once.
  def _RunningOnAndroid(self):
    options = self.__class__._original_finder_options.browser_options
    return options.browser_type.startswith('android')

  def _VerifyActiveAndInactiveGPUs(
      self, expected_active_gpu, expected_inactive_gpus):
    tab = self.tab
    basic_infos = tab.EvaluateJavaScript('browserBridge.gpuInfo.basic_info')
    active_gpu = []
    inactive_gpus = []
    index = 0
    for info in basic_infos:
      description = info['description']
      value = info['value']
      if description.startswith('GPU%d' % index) and value.startswith('VENDOR'):
        if value.endswith('*ACTIVE*'):
          active_gpu.append(value)
        else:
          inactive_gpus.append(value)
        index += 1
    if active_gpu != expected_active_gpu:
      self.fail('Active GPU field is wrong %s' % active_gpu)
    if inactive_gpus != expected_inactive_gpus:
      self.fail('Inactive GPU field is wrong %s' % inactive_gpus)

  ######################################
  # The actual tests

  def _GpuProcess_canvas2d(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([])
    self._NavigateAndWait(test_path)
    self._VerifyGpuProcessPresent()

  def _GpuProcess_css3d(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([])
    self._NavigateAndWait(test_path)
    self._VerifyGpuProcessPresent()

  def _GpuProcess_webgl(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([])
    self._NavigateAndWait(test_path)
    self._VerifyGpuProcessPresent()

  def _GpuProcess_video(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([])
    self._NavigateAndWait(test_path)
    self._VerifyGpuProcessPresent()

  def _GpuProcess_gpu_info_complete(self, test_path):
    # Regression test for crbug.com/454906
    self.RestartBrowserIfNecessaryWithArgs([])
    self._NavigateAndWait(test_path)
    tab = self.tab
    if not tab.browser.supports_system_info:
      self.fail('Browser must support system info')
    system_info = tab.browser.GetSystemInfo()
    if not system_info.gpu:
      self.fail('Target machine must have a GPU')
    if not system_info.gpu.aux_attributes:
      self.fail('Browser must support GPU aux attributes')
    if not 'gl_renderer' in system_info.gpu.aux_attributes:
      self.fail('Browser must have gl_renderer in aux attribs')
    if len(system_info.gpu.aux_attributes['gl_renderer']) <= 0:
      self.fail('Must have a non-empty gl_renderer string')

  def _GpuProcess_no_gpu_process(self, test_path):
    options = self.__class__._original_finder_options.browser_options
    if options.browser_type.startswith('android'):
      # Android doesn't support starting up the browser without any
      # GPU process. This test is skipped on Android in
      # gpu_process_expectations.py, but we must at least be able to
      # bring up the browser in order to detect that the test
      # shouldn't run. Faking a vendor and device ID can get the
      # browser into a state where it won't launch.
      return
    elif sys.platform in ('cygwin', 'win32'):
      # Hit id 34 from kSoftwareRenderingListEntries.
      self.RestartBrowserIfNecessaryWithArgs([
        '--gpu-testing-vendor-id=0x5333',
        '--gpu-testing-device-id=0x8811'])
    elif sys.platform.startswith('linux'):
      # Hit id 50 from kSoftwareRenderingListEntries.
      self.RestartBrowserIfNecessaryWithArgs([
        '--gpu-no-complete-info-collection',
        '--gpu-testing-vendor-id=0x10de',
        '--gpu-testing-device-id=0x0de1',
        '--gpu-testing-gl-vendor=VMware',
        '--gpu-testing-gl-renderer=softpipe',
        '--gpu-testing-gl-version="2.1 Mesa 10.1"'])
    elif sys.platform == 'darwin':
      # Hit id 112 from kSoftwareRenderingListEntries.
      self.RestartBrowserIfNecessaryWithArgs([
        '--gpu-testing-vendor-id=0x8086',
        '--gpu-testing-device-id=0x0116'])
    self._Navigate(test_path)
    if self.tab.EvaluateJavaScript('chrome.gpuBenchmarking.hasGpuProcess()'):
      self.fail('GPU process detected')

  def _GpuProcess_readback_webgl_gpu_process(self, test_path):
    # This test was designed to only run on desktop Linux.
    options = self.__class__._original_finder_options.browser_options
    is_platform_android = options.browser_type.startswith('android')
    if sys.platform.startswith('linux') and not is_platform_android:
      # Hit id 110 from kSoftwareRenderingListEntries.
      self.RestartBrowserIfNecessaryWithArgs([
        '--gpu-testing-vendor-id=0x10de',
        '--gpu-testing-device-id=0x0de1',
        '--gpu-testing-gl-vendor=VMware',
        '--gpu-testing-gl-renderer=Gallium 0.4 ' \
        'on llvmpipe (LLVM 3.4, 256 bits)',
        '--gpu-testing-gl-version="3.0 Mesa 11.2"'])
      self._Navigate(test_path)
      feature_status_list = self.tab.EvaluateJavaScript(
          'browserBridge.gpuInfo.featureStatus.featureStatus')
      result = True
      for name, status in feature_status_list.items():
        if name == 'multiple_raster_threads':
          result = result and status == 'enabled_on'
        elif name == 'native_gpu_memory_buffers':
          result = result and status == 'disabled_software'
        elif name == 'webgl':
          result = result and status == 'enabled_readback'
        elif name == 'webgl2':
          result = result and status == 'unavailable_off'
        elif name == 'checker_imaging':
          pass
        else:
          result = result and status == 'unavailable_software'
      if not result:
        self.fail('WebGL readback setup failed: %s' % feature_status_list)

  def _GpuProcess_skip_gpu_process(self, test_path):
    # This test loads functional_webgl.html so that there is a
    # deliberate attempt to use an API which would start the GPU
    # process. On platforms where SwiftShader is used, this test
    # should be skipped. Once SwiftShader is enabled on all platforms,
    # this test should be removed.
    self.RestartBrowserIfNecessaryWithArgs([
      '--disable-gpu',
      '--skip-gpu-data-loading'])
    self._NavigateAndWait(test_path)
    if self.tab.EvaluateJavaScript('chrome.gpuBenchmarking.hasGpuProcess()'):
      self.fail('GPU process detected')

  def _GpuProcess_identify_active_gpu1(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([
      '--gpu-testing-vendor-id=0x8086',
      '--gpu-testing-device-id=0x040a',
      '--gpu-testing-secondary-vendor-ids=0x10de',
      '--gpu-testing-secondary-device-ids=0x0de1',
      '--gpu-testing-gl-vendor=nouveau',
      '--disable-software-rasterizer'])
    self._Navigate(test_path)
    self._VerifyActiveAndInactiveGPUs(
      ['VENDOR = 0x10de, DEVICE= 0x0de1 *ACTIVE*'],
      ['VENDOR = 0x8086, DEVICE= 0x040a'])

  def _GpuProcess_identify_active_gpu2(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([
      '--gpu-testing-vendor-id=0x8086',
      '--gpu-testing-device-id=0x040a',
      '--gpu-testing-secondary-vendor-ids=0x10de',
      '--gpu-testing-secondary-device-ids=0x0de1',
      '--gpu-testing-gl-vendor=Intel',
      '--disable-software-rasterizer'])
    self._Navigate(test_path)
    self._VerifyActiveAndInactiveGPUs(
      ['VENDOR = 0x8086, DEVICE= 0x040a *ACTIVE*'],
      ['VENDOR = 0x10de, DEVICE= 0x0de1'])

  def _GpuProcess_identify_active_gpu3(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([
      '--gpu-testing-vendor-id=0x8086',
      '--gpu-testing-device-id=0x040a',
      '--gpu-testing-secondary-vendor-ids=0x10de;0x1002',
      '--gpu-testing-secondary-device-ids=0x0de1;0x6779',
      '--gpu-testing-gl-vendor=X.Org',
      '--gpu-testing-gl-renderer=AMD R600',
      '--disable-software-rasterizer'])
    self._Navigate(test_path)
    self._VerifyActiveAndInactiveGPUs(
      ['VENDOR = 0x1002, DEVICE= 0x6779 *ACTIVE*'],
      ['VENDOR = 0x8086, DEVICE= 0x040a',
       'VENDOR = 0x10de, DEVICE= 0x0de1'])

  def _GpuProcess_identify_active_gpu4(self, test_path):
    self.RestartBrowserIfNecessaryWithArgs([
      '--gpu-testing-vendor-id=0x10de',
      '--gpu-testing-device-id=0x0de1',
      '--gpu-testing-secondary-vendor-ids=',
      '--gpu-testing-secondary-device-ids=',
      '--gpu-testing-gl-vendor=nouveau',
      '--disable-software-rasterizer'])
    self._Navigate(test_path)
    self._VerifyActiveAndInactiveGPUs(
      ['VENDOR = 0x10de, DEVICE= 0x0de1 *ACTIVE*'],
      [])

  def _GpuProcess_swiftshader_for_webgl(self, test_path):
    # This test loads functional_webgl.html so that there is a
    # deliberate attempt to use an API which would start the GPU
    # process. On Windows, and eventually on other platforms where
    # SwiftShader is used, this test should pass.
    #
    args_list = ([
      # Hit id 4 from kSoftwareRenderingListEntries.
      '--gpu-testing-vendor-id=0x8086',
      '--gpu-testing-device-id=0x27A2'],
      # Explicitly disable GPU access.
     ['--disable-gpu'])
    for args in args_list:
      self.RestartBrowserIfNecessaryWithArgs(args)
      self._NavigateAndWait(test_path)
      # Validate the WebGL unmasked renderer string.
      renderer = self.tab.EvaluateJavaScript('gl_renderer')
      if not renderer:
        self.fail('getParameter(UNMASKED_RENDERER_WEBGL) was null')
      if 'SwiftShader' not in renderer:
        self.fail('Expected SwiftShader renderer; instead got ' + renderer)
      # Validate GPU info.
      if not self.browser.supports_system_info:
        self.fail("Browser doesn't support GetSystemInfo")
      gpu = self.browser.GetSystemInfo().gpu
      if not gpu:
        self.fail('Target machine must have a GPU')
      if not gpu.aux_attributes:
        self.fail('Browser must support GPU aux attributes')
      if not gpu.aux_attributes['software_rendering']:
        self.fail("Software rendering was disabled")
      if 'SwiftShader' not in gpu.aux_attributes['gl_renderer']:
        self.fail("Expected 'SwiftShader' in GPU info GL renderer string")
      if 'Google' not in gpu.aux_attributes['gl_vendor']:
        self.fail("Expected 'Google' in GPU info GL vendor string")
      device = gpu.devices[0]
      if not device:
        self.fail("System Info doesn't have a device")
      # Validate extensions.
      ext_list = [
        'ANGLE_instanced_arrays',
        'EXT_blend_minmax',
        'EXT_texture_filter_anisotropic',
        'WEBKIT_EXT_texture_filter_anisotropic',
        'OES_element_index_uint',
        'OES_standard_derivatives',
        'OES_texture_float',
        'OES_texture_float_linear',
        'OES_texture_half_float',
        'OES_texture_half_float_linear',
        'OES_vertex_array_object',
        'WEBGL_compressed_texture_etc1',
        'WEBGL_debug_renderer_info',
        'WEBGL_debug_shaders',
        'WEBGL_depth_texture',
        'WEBKIT_WEBGL_depth_texture',
        'WEBGL_draw_buffers',
        'WEBGL_lose_context',
        'WEBKIT_WEBGL_lose_context',
      ]
      tab = self.tab
      for ext in ext_list:
        if tab.EvaluateJavaScript('!gl_context.getExtension("' + ext + '")'):
          self.fail("Expected " + ext + " support")

def load_tests(loader, tests, pattern):
  del loader, tests, pattern  # Unused.
  return gpu_integration_test.LoadAllTestsInModule(sys.modules[__name__])
