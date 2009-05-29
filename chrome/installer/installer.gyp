{
  'variables': {
    'version_py': '../../chrome/tools/build/version.py',
    'VERSION': '../../chrome/VERSION',
    # 'BRANDING' is set in the 'conditions' section at the bottom.
  },
 'conditions': [
    [ 'branding == "Chrome"', {
      'variables': {
         'BRANDING': '../../chrome/app/theme/google_chrome/BRANDING',
      },
    }, { # else branding!="Chrome"
      'variables': {
         'BRANDING': '../../chrome/app/theme/chromium/BRANDING',
      },
    }],
  ],
  'includes': [
    '../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'installer_util',
       'conditions': [
         ['OS=="linux"', {
           'type': 'none',
           # Add these files to the build output so the build archives will be
           # "hermetic" for packaging. This is only for branding="Chrome" since
           # we only create packages for official builds.
           'conditions': [
             ['branding=="Chrome"', {
               'variables': {
                 'branding_path': 'google_chrome',
                 'lib32_dir': '<!(if uname -m | egrep -q "x86_64"; then echo lib32; else echo lib; fi)',
               },
               'copies': [
                 # Copy tools for generating packages from the build archive.
                 {
                   'destination': '<(PRODUCT_DIR)/installer/',
                   'files': [
                     'linux/internal/build_from_archive.sh',
                   ]
                 },
                 {
                   'destination': '<(PRODUCT_DIR)/installer/debian/',
                   'files': [
                     'linux/internal/debian/changelog.template',
                     'linux/internal/debian/postinst',
                     'linux/internal/debian/prerm',
                     'linux/internal/debian/build.sh',
                     'linux/internal/debian/control.template',
                   ]
                 },
                 {
                   'destination': '<(PRODUCT_DIR)/installer/common/',
                   'files': [
                     'linux/internal/common/apt.include',
                     'linux/internal/common/wrapper',
                     'linux/internal/common/updater',
                     'linux/internal/common/desktop.template',
                     'linux/internal/common/google-chrome/google-chrome.info',
                   ]
                 },
                 # System libs needed for 64-bit package building.
                 {
                   'destination': '<(PRODUCT_DIR)/installer/lib32/',
                   'files': [
                     '/usr/<(lib32_dir)/libsqlite3.so.0',
                     '/usr/<(lib32_dir)/libsqlite3.so.0.8.6',
                     '/usr/<(lib32_dir)/libnspr4.so.0d',
                     '/usr/<(lib32_dir)/libplds4.so.0d',
                     '/usr/<(lib32_dir)/libplc4.so.0d',
                     '/usr/<(lib32_dir)/libssl3.so.1d',
                     '/usr/<(lib32_dir)/libnss3.so.1d',
                     '/usr/<(lib32_dir)/libsmime3.so.1d',
                     '/usr/<(lib32_dir)/libnssutil3.so.1d',
                     '/usr/<(lib32_dir)/nss/libfreebl3.so',
                     '/usr/<(lib32_dir)/nss/libsoftokn3.chk',
                     '/usr/<(lib32_dir)/nss/libsoftokn3.so',
                     '/usr/<(lib32_dir)/nss/libnssckbi.so',
                     '/usr/<(lib32_dir)/nss/libnssdbm3.so',
                     '/usr/<(lib32_dir)/nss/libfreebl3.chk',
                   ],
                 },
                 # Additional theme resources needed for package building.
                 {
                   'destination': '<(PRODUCT_DIR)/installer/theme/',
                   'files': [
                     '../app/theme/<(branding_path)/product_logo_16.png',
                     '../app/theme/<(branding_path)/product_logo_32.png',
                     '../app/theme/<(branding_path)/product_logo_48.png',
                     '../app/theme/<(branding_path)/product_logo_256.png',
                     '../app/theme/<(branding_path)/BRANDING',
                   ],
                 },
               ],
               'actions': [
                 {
                   'action_name': 'save_build_info',
                   'inputs': [
                     '<(BRANDING)',
                     '<(VERSION)',
                   ],
                   'outputs': [
                     '<(PRODUCT_DIR)/installer/version.txt',
                   ],
                   # Jst output the default version info variables.
                   'action': ['python', '<(version_py)', '-f',
                              '<(BRANDING)', '-f', '<(VERSION)',
                              '-o', '<@(_outputs)'],
                 },
               ],
             }],
           ],
         }],
         ['OS=="win"', {
          'type': '<(library)',
          'dependencies': [
            'installer_util_strings',
            '../chrome.gyp:common',
            '../chrome.gyp:chrome_resources',
            '../chrome.gyp:chrome_strings',
            '../../net/net.gyp:net_resources',
            '../../media/media.gyp:media',
            '../../skia/skia.gyp:skia',
            '../../third_party/icu38/icu38.gyp:icui18n',
            '../../third_party/icu38/icu38.gyp:icuuc',
            '../../third_party/libxml/libxml.gyp:libxml',
            '../../third_party/lzma_sdk/lzma_sdk.gyp:lzma_sdk',
            '../../third_party/npapi/npapi.gyp:npapi',
            '../third_party/hunspell/hunspell.gyp:hunspell',
          ],
          'include_dirs': [
            '../..',
          ],
          'sources': [
            'util/browser_distribution.cc',
            'util/browser_distribution.h',
            'util/compat_checks.cc',
            'util/compat_checks.h',
            'util/copy_tree_work_item.cc',
            'util/copy_tree_work_item.h',
            'util/create_dir_work_item.cc',
            'util/create_dir_work_item.h',
            'util/create_reg_key_work_item.cc',
            'util/create_reg_key_work_item.h',
            'util/delete_reg_value_work_item.cc',
            'util/delete_reg_value_work_item.h',
            'util/delete_tree_work_item.cc',
            'util/delete_tree_work_item.h',
            'util/google_chrome_distribution.cc',
            'util/google_chrome_distribution.h',
            'util/google_update_constants.cc',
            'util/google_update_constants.h',
            'util/google_update_settings.cc',
            'util/google_update_settings.h',
            'util/helper.cc',
            'util/helper.h',
            'util/html_dialog.h',
            'util/html_dialog_impl.cc',
            'util/install_util.cc',
            'util/install_util.h',
            'util/l10n_string_util.cc',
            'util/l10n_string_util.h',
            'util/logging_installer.cc',
            'util/logging_installer.h',
            'util/lzma_util.cc',
            'util/lzma_util.h',
            'util/master_preferences.cc',
            'util/master_preferences.h',
            'util/move_tree_work_item.cc',
            'util/move_tree_work_item.h',
            'util/self_reg_work_item.cc',
            'util/self_reg_work_item.h',
            'util/set_reg_value_work_item.cc',
            'util/set_reg_value_work_item.h',
            'util/shell_util.cc',
            'util/shell_util.h',
            'util/util_constants.cc',
            'util/util_constants.h',
            'util/version.cc',
            'util/version.h',
            'util/work_item.cc',
            'util/work_item.h',
            'util/work_item_list.cc',
            'util/work_item_list.h',
          ],
       }],
     ],
    },
    {
      'target_name': 'installer_util_strings',
      'type': 'none',
      'actions': [
        {
          # TODO(sgk):  Clean this up so that we pass in the
          # file names to the script instead of having it hard-code
          # matching path names internally.
          'action_name': 'installer_util_strings',
          'inputs': [
            'util/prebuild/create_string_rc.py',
            '../app/generated_resources.grd',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/installer_util_strings/installer_util_strings.rc',
            '<(SHARED_INTERMEDIATE_DIR)/installer_util_strings/installer_util_strings.h',
          ],
          'action': [
            # The create_string_rc.py script requires the checked-in
            # python.exe that has google modules installed, and
            # a PYTHONPATH pointing to grit so it can import FP.
            # TODO:  clean this up
            'set PYTHONPATH=../../tools/grit/grit/extern', '&&',
            '../../third_party/python_24/python.exe',
            'util/prebuild/create_string_rc.py',
            '<(SHARED_INTERMEDIATE_DIR)/installer_util_strings'
          ],
          'msvs_cygwin_shell': 0,
        },
      ],
      'direct_dependent_settings': {
        'include_dirs': [
           '<(SHARED_INTERMEDIATE_DIR)/installer_util_strings',
        ],
      },
    },
  ],
}
