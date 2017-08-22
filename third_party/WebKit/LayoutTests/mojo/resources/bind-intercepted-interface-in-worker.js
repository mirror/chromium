importScripts('../../resources/testharness.js');
importScripts('file:///gen/layout_test_data/mojo/public/js/mojo_bindings.js');
importScripts('file:///gen/content/test/data/mojo_layouttest_test.mojom.js');

class MojoLayoutTestHelper {
  constructor() {
    this.bindingSet_ = new mojo.BindingSet(
        content.mojom.MojoLayoutTestHelper);
    this.interceptor_ = new MojoInterfaceInterceptor({
        interfaceName: content.mojom.MojoLayoutTestHelper.name});
    this.interceptor_.oninterfacerequest =
        e => this.bindingSet_.addBinding(this, e.handle);
    this.interceptor_.start();
  }

  getLastString() {
    return this.lastString_;
  }

  reverse(message) {
    this.lastString_ = message;
    return Promise.resolve({ reversed: message.split('').reverse().join('') });
  }
}

let mojoLayoutTestHelperImpl = new MojoLayoutTestHelper;

onmessage = async () => {
  let helper = new content.mojom.MojoLayoutTestHelperPtr;
  Mojo.bindInterface({ interfaceName: content.mojom.MojoLayoutTestHelper.name,
                       handle: mojo.makeRequest(helper).handle });

  let response = await helper.reverse('the string');
  assert_equals(response.reversed, 'gnirts eht');
  assert_equals(mojoLayoutTestHelperImpl.getLastString(), 'the string');
  postMessage('PASS');
};
