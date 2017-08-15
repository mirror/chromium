<style>
.note::before {
  content: 'Note: ';
  font-variant: small-caps;
  font-style: italic;
}

.doc h1 {
  margin: 0;
}
</style>

# Creating WebUI Interfaces
This guide is specific to creating a WebUI interface in 'src/components/'. To create a WebUI interface in `src/chrome/` follow [Creating Chrome WebUI Interfaces](https://www.chromium.org/developers/webui).

[TOC]

<a name="creating_webui_page"></a>
## Creating the WebUI page

WebUI resources in `components/` will be added in your specific project folder. Create a project folder `src/components/hello_world/`. When creating WebUI resources, follow the [Web Development Style Guide](https://chromium.googlesource.com/chromium/src/+/master/styleguide/web/web.md). For a sample WebUI page you could start with the following files:

`src/components/hello_world/hello_world.html:`
```html
<!DOCTYPE HTML>
<html i18n-values="dir:textdirection">
<head>
 <meta charset="utf-8">
 <title i18n-content="helloWorldTitle"></title>
 <link rel="stylesheet" href="hello_world.css">
 <script src="chrome://resources/js/cr.js"></script>
 <script src="chrome://resources/js/load_time_data.js"></script>
 <script src="chrome://resources/js/util.js"></script>
 <script src="strings.js"></script>
 <script src="hello_world.js"></script>
</head>
<body i18n-values=".style.fontFamily:fontfamily;.style.fontSize:fontsize">
  <h1 i18n-content="helloWorldTitle"></h1>
  <p id="welcome-message"></p>
<script src="chrome://resources/js/i18n_template.js"></script>
</body>
</html>
```

`src/components/hello_world/hello_world.css:`
```css
p {
  white-space: pre-wrap;
}
```

`src/components/hello_world/hello_world.js:`
```js
cr.define('hello_world', function() {
  'use strict';

  /**
   * Be polite and insert translated hello world strings for the user on loading.
   */
  function initialize() {
    $('welcome-message').textContent = loadTimeData.getStringF('welcomeMessage',
        loadTimeData.getString('userName'));
  }

  // Return an object with all of the exports.
  return {
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', hello_world.initialize);
```

## Adding the resources

Resources files are added by creating a new file `hello_world_resources.grdp` in `components/resources/` The following additions add our `hello_world` files:

```xml
<?xml version="1.0" encoding="utf-8"?>
 <grit-part>
    <include name="IDR_HELLO_WORLD_HTML" file="../../components/hello_world/hello_world.html" type="BINDATA" />
   <include name="IDR_HELLO_WORLD_CSS" file="../../components/hello_world/hello_world.css" type="BINDATA" />
<include name="IDR_HELLO_WORLD_JS" file="../../components/hello_world/hello_world.js" type="BINDATA" />
  </grit-part>
```

## Adding URL constants for new chrome URL

Create the `constants.cc` and `constants.h` files to add the URL constants. This is where you will add the URL or URL's which will be directed to your new resources.

`src/components/hello_world/constants.cc:`
```c++
+ const char kChromeUIHelloWorldURL[] = "chrome://hello-world/";
...
+ const char kChromeUIHelloWorldHost[] = "hello-world";
```

`src/components/hello_world/constants.h:`
```c++
+ extern const char kChromeUIHelloWorldURL[];
...
+ extern const char kChromeUIHelloWorldHost[];
```

## Adding localized strings

We need a few string resources for translated strings to work on the new resource. The welcome message contains a variable with a sample value so that it can be accurately translated. Create a new file `components/hello_world_strings.grdp`. You can add the messages as follow:

```xml
+ <message name="IDS_HELLO_WORLD_TITLE" desc="A happy message saying hello to the world">
+   Hello World!
+ </message>
+ <message name="IDS_HELLO_WORLD_WELCOME_TEXT" desc="Message welcoming the user to the hello world page">
+   Welcome to this fancy Hello World page <ph name="WELCOME_NAME">$1<ex>Chromium User</ex></ph>!
+ </message>
```

## Adding a WebUI class for handling requests to the chrome://hello-world/ URL

Next we need a class to handle requests to this new resource URL. Typically this will subclass `ChromeWebUI` (but WebUI dialogs should subclass `HtmlDialogUI` instead).

`src/components/hello_world/hello_world_ui.cc:`
```c++
#include "components/hello_world/hello_world_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

HelloWorldUI::HelloWorldUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://hello-world source.
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIHelloWorldHost);

  // Localized strings.
  html_source->AddLocalizedString("helloWorldTitle", IDS_HELLO_WORLD_TITLE);
  html_source->AddLocalizedString("welcomeMessage", IDS_HELLO_WORLD_WELCOME_TEXT);

  // As a demonstration of passing a variable for JS to use we pass in the name "Bob".
  html_source->AddString("userName", "Bob");
  html_source->SetJsonPath("strings.js");

  // Add required resources.
  html_source->AddResourcePath("hello_world.css", IDR_HELLO_WORLD_CSS);
  html_source->AddResourcePath("hello_world.js", IDR_HELLO_WORLD_JS);
  html_source->SetDefaultResource(IDR_HELLO_WORLD_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

HelloWorldUI::~HelloWorldUI() {
}
```
`src/components/hello_world/hello_world_ui.h:`
```c++
#ifndef CHROME_BROWSER_UI_WEBUI_HELLO_WORLD_UI_H_
#define CHROME_BROWSER_UI_WEBUI_HELLO_WORLD_UI_H_
#pragma once

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

// The WebUI for chrome://hello-world
class HelloWorldUI : public content::WebUIController {
 public:
  explicit HelloWorldUI(content::WebUI* web_ui);
  ~HelloWorldUI() override;
 private:
  DISALLOW_COPY_AND_ASSIGN(HelloWorldUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_HELLO_WORLD_UI_H_
```

## Adding new sources to Chrome

In order for your new class to be built and linked in, it needs to be added to the project gypi file. Create new `BUILD.gn` and `DEPS` files inside `src/components/hello_world/`.

`src/components/hello_world/BUILD.gn:`
```
sources = [
...
+   "hello_worldi/hello_world_ui.cc",
+   "hello_world/hello_world_ui.h",
```
 `src/components/hello_world/DEPS:`
 ```
 include_rules = [
    "+components/grit/components_resources.h",
    "+components/strings/grit/components_strings.h",
    "+components/grit/components_scaled_resources.h"
  ]
```

## Adding your WebUI request handler to the Chrome WebUI factory

The Chrome WebUI factory is where you setup your new request handler.

`src/chrome/browser/ui/webui/chrome_web_ui_controller_factory.cc:`
```c++
+ #include "components/hello_world/hello_world_ui.h"
+ #include "components/hello_world/constants.h"
...
+ if (url.host() == chrome::kChromeUIHelloWorldHost)
+   return &NewWebUI<HelloWorldUI>;
```

## Testing

You're done! Assuming no errors (because everyone gets their code perfect the first time) you should be able to compile and run chrome and navigate to `chrome://hello-world/` and see your nifty welcome text!

## Adding a callback handler

You probably want your new WebUI page to be able to do something or get information from the C++ world. For this, we use message callback handlers. Let's say that we don't trust the Javascript engine to be able to add two integers together (since we know that it uses floating point values internally). We could add a callback handler to perform integer arithmetic for us.

`src/components/hello_world/hello_world_ui.h:`
```c++
#include "content/public/browser/web_ui.h"
+
+ namespace base {
+   class ListValue;
+ }  // namespace base

// The WebUI for chrome://hello-world
...
    // Set up the chrome://hello-world source.
    content::WebUIDataSource* html_source = content::WebUIDataSource::Create(hello_world::kChromeUIHelloWorldHost);
+
+   // Register callback handler.
+   RegisterMessageCallback("addNumbers",
+       base::Bind(&HelloWorldUI::AddNumbers,
+                  base::Unretained(this)));

    // Localized strings.
...
    virtual ~HelloWorldUI();
+
+  private:
+   // Add two numbers together using integer arithmetic.
+   void AddNumbers(const base::ListValue* args);

    DISALLOW_COPY_AND_ASSIGN(HelloWorldUI);
  };
```

`src/components/hello_world/hello_world_ui.cc:`
```c++
  #include "components/hello_world/hello_world_ui.h"
+
+ #include "base/values.h"
  #include "chrome/browser/profiles/profile.h"
...
  HelloWorldUI::~HelloWorldUI() {
  }
+
+ void HelloWorldUI::AddNumbers(const base::ListValue* args) {
+   int term1, term2;
+   if (!args->GetInteger(0, &term1) || !args->GetInteger(1, &term2))
+     return;
+   base::FundamentalValue result(term1 + term2);
+   CallJavascriptFunction("hello_world.addResult", result);
+ }
```

`src/components/hello_world/hello_world.js:`
```c++
    function initialize() {
+     chrome.send('addNumbers', [2, 2]);
    }
+
+   function addResult(result) {
+     alert('The result of our C++ arithmetic: 2 + 2 = ' + result);
+   }

    return {
+     addResult: addResult,
      initialize: initialize,
    };
```

You'll notice that the call is asynchronous. We must wait for the C++ side to call our Javascript function to get the result.

## Creating a WebUI Dialog

Creating a WebUI dialogue c++->js and js->c++ is described in [WebUI Explainer](https://chromium.googlesource.com/chromium/src/+/master/docs/webui_explainer.md).


<script>
let nameEls = Array.from(document.querySelectorAll('[id], a[name]'));
let names = nameEls.map(nameEl => nameEl.name || nameEl.id);

let localLinks = Array.from(document.querySelectorAll('a[href^="#"]'));
let hrefs = localLinks.map(a => a.href.split('#')[1]);

hrefs.forEach(href => {
  if (names.includes(href))
    console.info('found: ' + href);
  else
    console.error('broken href: ' + href);
})
</script>
