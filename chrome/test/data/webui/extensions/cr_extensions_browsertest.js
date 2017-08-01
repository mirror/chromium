// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Settings tests. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE([
    ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "base/command_line.h"');
GEN('#include "chrome/browser/ui/webui/extensions/' +
    'extension_settings_browsertest.h"');

/**
 * Basic test fixture for the MD chrome://extensions page. Installs no
 * extensions.
 * @constructor
 * @extends {PolymerTest}
 */
function CrExtensionsBrowserTest() {}

CrExtensionsBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://extensions/',

  /** @override */
  commandLineSwitches: [{
    switchName: 'enable-features',
    switchValue: 'MaterialDesignExtensions',
  }],

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    ROOT_PATH + 'ui/webui/resources/js/assert.js',
    'extension_test_util.js',
    'extension_code_section_test.js',
    'extension_error_page_test.js',
    'extension_keyboard_shortcuts_test.js',
    'extension_options_dialog_test.js',
    'extension_pack_dialog_test.js',
    'extension_navigation_helper_test.js',
    'extension_shortcut_input_test.js',
    'extension_manager_test.js',
    '../mock_controller.js',
    '../../../../../ui/webui/resources/js/promise_resolver.js',
    '../../../../../ui/webui/resources/js/webui_resource_test.js',
  ]),

  /** @override */
  typedefCppFixture: 'ExtensionSettingsUIBrowserTest',
};

/**
 * Test fixture with one installed extension.
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsBrowserTestWithInstalledExtension() {}

CrExtensionsBrowserTestWithInstalledExtension.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  testGenPreamble: function() {
    GEN('  InstallGoodExtension();');
    GEN('  SetAutoConfirmUninstall();');
  },
};

/**
 * Test fixture that navigates to chrome://extensions/?id=<id>.
 * @constructor
 * @extends {CrExtensionsBrowserTestWithInstalledExtension}
 */
function CrExtensionsBrowserTestWithIdQueryParam() {}

CrExtensionsBrowserTestWithIdQueryParam.prototype = {
  __proto__: CrExtensionsBrowserTestWithInstalledExtension.prototype,

  /** @override */
  browsePreload: 'chrome://extensions/?id=ldnnhddmnhbkjipkidpdiheffobcpfmf',
};

/**
 * Test fixture with multiple installed extensions of different types.
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsBrowserTestWithMultipleExtensionTypesInstalled() {}

CrExtensionsBrowserTestWithMultipleExtensionTypesInstalled.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  testGenPreamble: function() {
    GEN('  InstallGoodExtension();');
    GEN('  InstallPackagedApp();');
    GEN('  InstallHostedApp();');
    GEN('  InstallPlatformApp();');
  },
};

////////////////////////////////////////////////////////////////////////////////
// Extension Sidebar Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsSidebarTest() {}

CrExtensionsSidebarTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_sidebar_test.js',
  ]),
};

TEST_F(
    'CrExtensionsSidebarTest', 'ExtensionSidebarLayoutAndClickHandlersTest',
    function() {
      mocha
          .grep(
              assert(extension_sidebar_tests.TestNames.LayoutAndClickHandlers))
          .run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Sidebar Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsToolbarTest() {}

CrExtensionsToolbarTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_toolbar_test.js',
  ]),
};

TEST_F('CrExtensionsToolbarTest', 'ExtensionToolbarLayoutTest', function() {
  mocha.grep(assert(extension_toolbar_tests.TestNames.Layout)).run();
});

TEST_F(
    'CrExtensionsToolbarTest', 'ExtensionToolbarClickHandlersTest', function() {
      mocha.grep(assert(extension_toolbar_tests.TestNames.ClickHandlers)).run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Item Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsItemsTest() {}

CrExtensionsItemsTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_item_test.js',
  ]),
};

TEST_F('CrExtensionsItemsTest', 'ExtensionItemNormalStateTest', function() {
  var TestNames = extension_item_tests.TestNames;
  mocha.grep(assert(TestNames.ElementVisibilityNormalState)).run();
});

TEST_F('CrExtensionsItemsTest', 'ExtensionItemDeveloperStateTest', function() {
  var TestNames = extension_item_tests.TestNames;
  mocha.grep(assert(TestNames.ElementVisibilityDeveloperState)).run();
});

TEST_F('CrExtensionsItemsTest', 'ExtensionItemClickableItemsTest', function() {
  var TestNames = extension_item_tests.TestNames;
  mocha.grep(assert(TestNames.ClickableItems)).run();
});

TEST_F('CrExtensionsItemsTest', 'ExtensionItemWarningsTest', function() {
  mocha.grep(assert(extension_item_tests.TestNames.Warnings)).run();
});

TEST_F('CrExtensionsItemsTest', 'ExtensionItemSourceIndicatorTest', function() {
  mocha.grep(assert(extension_item_tests.TestNames.SourceIndicator)).run();
});

TEST_F('CrExtensionsItemsTest', 'ExtensionItemEnableToggleTest', function() {
  mocha.grep(assert(extension_item_tests.TestNames.EnableToggle)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Detail View Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsDetailViewTest() {}

CrExtensionsDetailViewTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_detail_view_test.js',
  ]),
};

TEST_F(
    'CrExtensionsDetailViewTest', 'ExtensionDetailViewLayoutTest', function() {
      mocha.grep(assert(extension_detail_view_tests.TestNames.Layout)).run();
    });

TEST_F(
    'CrExtensionsDetailViewTest', 'ExtensionDetailViewClickableElementsTest',
    function() {
      mocha
          .grep(assert(extension_detail_view_tests.TestNames.ClickableElements))
          .run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Item List Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsItemListTest() {}

CrExtensionsItemListTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_item_list_test.js',
  ]),
};

TEST_F('CrExtensionsItemListTest', 'ExtensionItemList', function() {
  mocha.grep(
      assert(extension_item_list_tests.TestNames.ItemListFiltering)).run();
});

TEST_F('CrExtensionsItemListTest', 'ExtensionItemListEmpty', function() {
  mocha.grep(assert(extension_item_list_tests.TestNames.ItemListNoItemsMsg))
      .run();
});

TEST_F(
    'CrExtensionsItemListTest', 'ExtensionItemListNoSearchResults', function() {
      mocha
          .grep(assert(
              extension_item_list_tests.TestNames.ItemListNoSearchResultsMsg))
          .run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Load Error Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsLoadErrorTest() {}

CrExtensionsLoadErrorTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTest.prototype.extraLibraries.concat([
    'extension_load_error_test.js',
  ]),
};

TEST_F(
    'CrExtensionsLoadErrorTest', 'ExtensionLoadErrorInteractionTest',
    function() {
      mocha.grep(assert(extension_load_error_tests.TestNames.Interaction))
          .run();
    });

TEST_F(
    'CrExtensionsLoadErrorTest', 'ExtensionLoadErrorCodeSectionTest',
    function() {
      mocha.grep(assert(extension_load_error_tests.TestNames.CodeSection))
          .run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Service Tests

/**
 * @constructor
 * @extends {CrExtensionsBrowserTest}
 */
function CrExtensionsServiceTest() {}

CrExtensionsServiceTest.prototype = {
  __proto__: CrExtensionsBrowserTestWithInstalledExtension.prototype,

  /** @override */
  extraLibraries: CrExtensionsBrowserTestWithInstalledExtension.prototype
                      .extraLibraries.concat([
                        'extension_service_test.js',
                      ]),
};

TEST_F(
    'CrExtensionsServiceTest', 'ExtensionServiceToggleEnableTest', function() {
      mocha.grep(assert(extension_service_tests.TestNames.EnableAndDisable))
          .run();
    });
TEST_F(
    'CrExtensionsServiceTest', 'ExtensionServiceToggleIncognitoTest',
    function() {
      mocha.grep(assert(extension_service_tests.TestNames.ToggleIncognitoMode))
          .run();
    });
TEST_F('CrExtensionsServiceTest', 'ExtensionServiceUninstallTest', function() {
  mocha.grep(assert(extension_service_tests.TestNames.Uninstall)).run();
});
TEST_F(
    'CrExtensionsServiceTest', 'ExtensionServiceProfileSettingsTest',
    function() {
      mocha.grep(assert(extension_service_tests.TestNames.ProfileSettings))
          .run();
    });

////////////////////////////////////////////////////////////////////////////////
// Extension Manager Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionManagerItemOrderTest', function() {
  extension_manager_tests.registerTests();
  mocha.grep(assert(extension_manager_tests.TestNames.ItemOrder)).run();
});

TEST_F('CrExtensionsBrowserTestWithMultipleExtensionTypesInstalled',
       'ExtensionManagerItemListVisibilityTest', function() {
  extension_manager_tests.registerTests();
  mocha.grep(
      assert(extension_manager_tests.TestNames.ItemListVisibility)).run();
});

TEST_F('CrExtensionsBrowserTestWithMultipleExtensionTypesInstalled',
       'ExtensionManagerShowItemsTest', function() {
  extension_manager_tests.registerTests();
  mocha.grep(assert(extension_manager_tests.TestNames.ShowItems)).run();
});

TEST_F('CrExtensionsBrowserTestWithMultipleExtensionTypesInstalled',
       'ExtensionManagerChangePagesTest', function() {
  extension_manager_tests.registerTests();
  mocha.grep(assert(extension_manager_tests.TestNames.ChangePages)).run();
});

TEST_F('CrExtensionsBrowserTestWithIdQueryParam',
       'ExtensionManagerNavigationToDetailsTest', function() {
  extension_manager_tests.registerTests();
  mocha.grep(
      assert(extension_manager_tests.TestNames.UrlNavigationToDetails)).run();
});

TEST_F('CrExtensionsBrowserTest', 'ExtensionManagerUpdateItemDataTest',
       function() {
  extension_manager_tests.registerTests();
  mocha.grep(assert(extension_manager_tests.TestNames.UpdateItemData)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Keyboard Shortcuts Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionKeyboardShortcutsLayoutTest',
       function() {
  extension_keyboard_shortcut_tests.registerTests();
  mocha.grep(assert(extension_keyboard_shortcut_tests.TestNames.Layout)).run();
});

TEST_F('CrExtensionsBrowserTest', 'ExtensionShortcutUtilTest', function() {
  extension_keyboard_shortcut_tests.registerTests();
  mocha.grep(
      assert(extension_keyboard_shortcut_tests.TestNames.ShortcutUtil)).run();
});

TEST_F('CrExtensionsBrowserTest', 'ExtensionShortcutInputTest', function() {
  extension_shortcut_input_tests.registerTests();
  mocha.grep(
      assert(extension_shortcut_input_tests.TestNames.Basic)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Pack Dialog Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionPackDialogInteractionTest',
       function() {
  extension_pack_dialog_tests.registerTests();
  mocha.grep(assert(extension_pack_dialog_tests.TestNames.Interaction)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Options Dialog Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionOptionsDialogInteractionTest',
       function() {
  extension_options_dialog_tests.registerTests();
  mocha.grep(assert(extension_options_dialog_tests.TestNames.Layout)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Error Page Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionErrorPageLayoutTest',
       function() {
  extension_error_page_tests.registerTests();
  mocha.grep(assert(extension_error_page_tests.TestNames.Layout)).run();
});

TEST_F('CrExtensionsBrowserTest', 'ExtensionErrorPageCodeSectionTest',
       function() {
  extension_error_page_tests.registerTests();
  mocha.grep(assert(extension_error_page_tests.TestNames.CodeSection)).run();
});

TEST_F('CrExtensionsBrowserTest', 'ExtensionErrorPageErrorSelectionTest',
       function() {
  extension_error_page_tests.registerTests();
  mocha.grep(assert(extension_error_page_tests.TestNames.ErrorSelection)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Code Section Tests

TEST_F('CrExtensionsBrowserTest', 'ExtensionCodeSectionLayoutTest',
       function() {
  extension_code_section_tests.registerTests();
  mocha.grep(assert(extension_code_section_tests.TestNames.Layout)).run();
});

////////////////////////////////////////////////////////////////////////////////
// Extension Navigation Helper Tests

function CrExtensionsNavigationHelperBrowserTest() {}

// extensions.NavigationHelper observes window.location. In order to test this
// without the "real" NavigationHelper joining the party, we navigate to
// navigation_helper.html directly.
CrExtensionsNavigationHelperBrowserTest.prototype = {
  __proto__: CrExtensionsBrowserTest.prototype,

  /** @override */
  browsePreload: 'chrome://extensions/navigation_helper.html',
};

TEST_F('CrExtensionsNavigationHelperBrowserTest',
       'ExtensionNavigationHelperBasicTest', function() {
  extension_navigation_helper_tests.registerTests();
  mocha.grep(assert(extension_navigation_helper_tests.TestNames.Basic)).run();
});

TEST_F('CrExtensionsNavigationHelperBrowserTest',
       'ExtensionNavigationHelperConversionTest', function() {
  extension_navigation_helper_tests.registerTests();
  mocha.grep(
      assert(extension_navigation_helper_tests.TestNames.Conversions)).run();
});

TEST_F('CrExtensionsNavigationHelperBrowserTest',
       'ExtensionNavigationHelperPushAndReplaceStateTest', function() {
  extension_navigation_helper_tests.registerTests();
  mocha.grep(
      assert(extension_navigation_helper_tests.TestNames.PushAndReplaceState))
          .run();
});
