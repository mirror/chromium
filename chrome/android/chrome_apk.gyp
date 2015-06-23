# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'includes': [
    '../../chrome/chrome_android_paks.gypi', # Included for the list of pak resources.
    '../../build/util/version.gypi'
   ],
  'variables': {
    'chromium_code': 1,
    'package_name': 'chrome_public_apk',
    'manifest_package': 'org.chromium.chrome',
    'chrome_public_apk_manifest': '<(SHARED_INTERMEDIATE_DIR)/chrome_public_apk_manifest/AndroidManifest.xml',
    'chrome_public_test_apk_manifest': '<(SHARED_INTERMEDIATE_DIR)/chrome_public_test_apk_manifest/AndroidManifest.xml',
    'chrome_java_dir': 'java_staging',
    'chrome_java_tests_dir': 'javatests',
    'chrome_java_test_support_dir': '../test/android/javatests_staging',
    'chrome_native_sources_dir': '../browser/android/',
    # This list is shared with GN.
    # Defines a list of source files should be present in the open-source
    # chrome-apk but not in the published static_library which is included in the
    # real chrome for android.
    'chrome_public_app_native_sources': [
      '<(chrome_native_sources_dir)/chrome_entry_point.cc',
      '<(chrome_native_sources_dir)/chrome_main_delegate_staging_android_initializer.cc',
      '<(chrome_native_sources_dir)/chrome_staging_jni_onload.cc',
      '<(chrome_native_sources_dir)/chrome_staging_jni_onload.h',
    ],
  },
  'targets': [
     {
      # TODO(newt): delete this once all references are gone.
      'target_name': 'libchrome_staging',
      'type': 'none',
    },
    {
      # GN: //chrome/android:chrome_staging_java
      'target_name': 'chrome_staging_java',
      'type': 'none',
      'variables': {
        'java_in_dir': '<(chrome_java_dir)',
      },
      'dependencies': [
        'custom_tabs_service_aidl',
        '<(DEPTH)/base/base.gyp:base_java',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java',
        '<(DEPTH)/chrome/chrome.gyp:document_tab_model_info_proto_java',
        '<(DEPTH)/components/components.gyp:app_restrictions_resources',
        '<(DEPTH)/components/components.gyp:navigation_interception_java',
        '<(DEPTH)/components/components.gyp:service_tab_launcher',
        '<(DEPTH)/components/components.gyp:web_contents_delegate_android_java',
        '<(DEPTH)/content/content.gyp:content_java',
        '<(DEPTH)/media/media.gyp:media_java',
        '<(DEPTH)/net/net.gyp:net_java',
        '<(DEPTH)/third_party/android_protobuf/android_protobuf.gyp:protobuf_nano_javalib',
        '<(DEPTH)/third_party/android_tools/android_tools.gyp:android_support_v13_javalib',
        '<(DEPTH)/third_party/android_tools/android_tools.gyp:android_support_v7_appcompat_javalib',
        '<(DEPTH)/third_party/android_tools/android_tools.gyp:android_support_v7_mediarouter_javalib',
        '<(DEPTH)/third_party/android_tools/android_tools.gyp:android_support_v7_recyclerview_javalib',
        '<(DEPTH)/third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_javalib',
        '<(DEPTH)/third_party/jsr-305/jsr-305.gyp:jsr_305_javalib',
        '<(DEPTH)/ui/android/ui_android.gyp:ui_java',
      ],
      'conditions': [
        ['configuration_policy != 1', {
          'dependencies!': [
            '<(DEPTH)/components/components.gyp:app_restrictions_resources',
          ],
        }],
      ],
      'includes': [ '../../build/java.gypi' ],
    },
    {
      # GN: //chrome/test/android:chrome_staging_test_support_java
      'target_name': 'chrome_staging_test_support_java',
      'type': 'none',
      'variables': {
          'java_in_dir': '<(chrome_java_test_support_dir)',
      },
      'dependencies': [
        'chrome_staging_java',
        '<(DEPTH)/base/base.gyp:base_java',
        '<(DEPTH)/base/base.gyp:base_java_test_support',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java_test_support',
        '<(DEPTH)/content/content_shell_and_tests.gyp:content_java_test_support',
        '<(DEPTH)/net/net.gyp:net_java',
        '<(DEPTH)/net/net.gyp:net_java_test_support',
        '<(DEPTH)/sync/sync.gyp:sync_java_test_support',
      ],
      'includes': [ '../../build/java.gypi' ],
    },
    {
      # GN: //chrome/android:custom_tabs_service_aidl
      'target_name': 'custom_tabs_service_aidl',
      'type': 'none',
      'variables': {
        'aidl_interface_file': '<(chrome_java_dir)/src/org/chromium/chrome/browser/customtabs/common.aidl',
        'aidl_import_include': '<(chrome_java_dir)/src/org/chromium/chrome/browser/customtabs',
      },
      'sources': [
        '<(chrome_java_dir)/src/org/chromium/chrome/browser/customtabs/ICustomTabsConnectionCallback.aidl',
        '<(chrome_java_dir)/src/org/chromium/chrome/browser/customtabs/ICustomTabsConnectionService.aidl',
      ],
      'includes': [ '../../build/java_aidl.gypi' ],
    },
    {
      # GN: //chrome/android:chrome_public_template_resources
      'target_name': 'chrome_public_template_resources',
      'type': 'none',
      'variables': {
        'jinja_inputs_base_dir': 'java/res_template',
        'jinja_inputs': [
          '<(jinja_inputs_base_dir)/xml/searchable.xml',
          '<(jinja_inputs_base_dir)/xml/syncadapter.xml',
        ],
        'jinja_outputs_zip': '<(PRODUCT_DIR)/res.java/<(_target_name).zip',
        'jinja_variables': [
          'manifest_package=<(manifest_package)',
        ],
      },
      'all_dependent_settings': {
        'variables': {
          'additional_input_paths': ['<(jinja_outputs_zip)'],
          'dependencies_res_zip_paths': ['<(jinja_outputs_zip)'],
        },
      },
      'includes': [ '../../build/android/jinja_template.gypi' ],
    },
    {
      # GN: //chrome/android:chrome_public
      'target_name': 'libchrome_public',
      'type': 'shared_library',
      'dependencies': [
        '<(DEPTH)/chrome/chrome.gyp:chrome_android_core',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        '<@(chrome_public_app_native_sources)',
      ],
      'ldflags': [
        # Some android targets still depend on --gc-sections to link.
        # TODO: remove --gc-sections for Debug builds (crbug.com/159847).
        '-Wl,--gc-sections',
      ],
      'conditions': [
        # TODO(yfriedman): move this DEP to chrome_android_core to be shared
        # between internal/external.
        ['cld_version==2', {
          'dependencies': [
            '<(DEPTH)/third_party/cld_2/cld_2.gyp:cld2_dynamic',
          ],
        }],
        # conditions for order_text_section
        # Cygprofile methods need to be linked into the instrumented build.
        ['order_profiling!=0', {
          'conditions': [
            ['OS=="android"', {
              'dependencies': [ '<(DEPTH)/tools/cygprofile/cygprofile.gyp:cygprofile' ],
            }],
          ],
        }],  # order_profiling!=0
        ['use_allocator!="none"', {
          'dependencies': [
            '<(DEPTH)/base/allocator/allocator.gyp:allocator',
          ],
        }],
      ],
    },
    {
      # GN: //chrome/android:chrome_public_apk_manifest
      'target_name': 'chrome_public_manifest',
      'type': 'none',
      'variables': {
        'jinja_inputs': ['java/AndroidManifest.xml'],
        'jinja_output': '<(chrome_public_apk_manifest)',
        'jinja_variables': [
          'channel=<(android_channel)',
          'configuration_policy=<(configuration_policy)',
          'manifest_package=<(manifest_package)',
          'min_sdk_version=16',
          'target_sdk_version=22',
        ],
      },
      'includes': [ '../../build/android/jinja_template.gypi' ],
    },
    {
      # GN: //chrome/android:chrome_public_apk
      'target_name': 'chrome_public_apk',
      'type': 'none',
      'variables': {
        'android_manifest_path': '<(chrome_public_apk_manifest)',
        'apk_name': 'ChromePublic',
        'native_lib_target': 'libchrome_public',
        'java_in_dir': '<(chrome_java_dir)',
        'conditions': [
          # Only attempt loading the library from the APK for 64 bit devices
          # until the number of 32 bit devices which don't support this
          # approach falls to a minimal level -  http://crbug.com/390618.
          ['component != "shared_library" and profiling==0 and (target_arch == "arm64" or target_arch == "x86_64")', {
            'load_library_from_zip_file': '<(chrome_apk_load_library_from_zip)',
            'load_library_from_zip': '<(chrome_apk_load_library_from_zip)',
          }],
        ],
      },
      'dependencies': [
        'chrome_android_paks_copy',
        'chrome_public_template_resources',
        'chrome_staging_java',
      ],
      'includes': [ 'chrome_apk.gypi' ],
    },
    {
      # GN: N/A
      # chrome_public_apk creates a .jar as a side effect. Any java targets
      # that need that .jar in their classpath should depend on this target,
      'target_name': 'chrome_public_apk_java',
      'type': 'none',
      'dependencies': [
        'chrome_public_apk',
      ],
      'includes': [ '../../build/apk_fake_jar.gypi' ],
    },
    {
      # TODO(newt): delete this target once all references are gone.
      'target_name': 'chrome_staging_unittests',
      'type': 'static_library',
      'sources': [
        # This file is already tested elsewhere but is included here to
        # prevent this from being an empty static_library, which would
        # break the the test APK that depends on this.
        '../browser/android/policy/policy_manager_unittest.cc',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base_java',
        '<(DEPTH)/chrome/chrome.gyp:browser',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java',
        '<(DEPTH)/chrome/chrome.gyp:test_support_unit',
        '<(DEPTH)/net/net.gyp:net_test_support',
        '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
    },
    {
      # GN: None.
      # This target is for sharing tests between both upstream and internal
      # trees until sufficient test coverage is upstream.
      'target_name': 'chrome_shared_test_java',
      'type': 'none',
      'variables': {
        'java_in_dir': '<(chrome_java_tests_dir)',
      },
      'dependencies': [
        'chrome_staging_java',
        'chrome_staging_test_support_java',
        '<(DEPTH)/base/base.gyp:base_java',
        '<(DEPTH)/base/base.gyp:base_java_test_support',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java',
        '<(DEPTH)/chrome/chrome.gyp:chrome_java_test_support',
        '<(DEPTH)/components/components.gyp:web_contents_delegate_android_java',
        '<(DEPTH)/content/content_shell_and_tests.gyp:content_java_test_support',
        '<(DEPTH)/net/net.gyp:net_java',
        '<(DEPTH)/net/net.gyp:net_java_test_support',
        '<(DEPTH)/sync/sync.gyp:sync_java_test_support',
        '<(DEPTH)/third_party/android_tools/android_tools.gyp:android_support_v7_appcompat_javalib',
      ],
      'includes': [ '../../build/java.gypi' ],
    },
    {
      # GN: //chrome/android:chrome_public_test_apk_manifest
      'target_name': 'chrome_public_test_apk_manifest',
      'type': 'none',
      'variables': {
        'jinja_inputs': ['<(chrome_java_tests_dir)/AndroidManifest.xml'],
        'jinja_output': '<(chrome_public_test_apk_manifest)',
        'jinja_variables': [
          'manifest_package=<(manifest_package)',
        ],
      },
      'includes': [ '../../build/android/jinja_template.gypi' ],
    },
    {
      # GN: //chrome/android:chrome_public_test_apk
      'target_name': 'chrome_public_test_apk',
      'type': 'none',
      'dependencies': [
        'chrome_shared_test_java',
        'chrome_public_apk_java',
        '<(DEPTH)/testing/android/on_device_instrumentation.gyp:broker_java',
        '<(DEPTH)/testing/android/on_device_instrumentation.gyp:require_driver_apk',
      ],
      'variables': {
        'android_manifest_path': '<(chrome_public_test_apk_manifest)',
        'package_name': 'chrome_public_test',
        'java_in_dir': '<(chrome_java_tests_dir)',
        'java_in_dir_suffix': '/src_dummy',
        'apk_name': 'ChromePublicTest',
        'is_test_apk': 1,
        'test_type': 'instrumentation',
        'isolate_file': '../chrome_public_test_apk.isolate',
      },
      'includes': [
        '../../build/java_apk.gypi',
        '../../build/android/test_runner.gypi',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
