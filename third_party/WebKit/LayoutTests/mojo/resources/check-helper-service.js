importScripts('../../resources/testharness.js');
importScripts('file:///gen/layout_test_data/mojo/public/js/mojo_bindings.js');
importScripts('file:///gen/content/test/data/mojo_layouttest_test.mojom.js');

let helper = new content.mojom.MojoLayoutTestHelperPtr;
Mojo.bindInterface({ serviceName: "layout_test_helper_service",
                     interfaceName: content.mojom.MojoLayoutTestHelper.name,
                     handle: mojo.makeRequest(helper).handle });

helper.reverse('the string').then(response => {
  assert_equals(response.reversed, 'gnirts eht');
  postMessage('PASS');
});
