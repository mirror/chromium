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

[TOC]

<a name="creating_webui_page"></a>
## Creating the WebUI page

WebUI resources in `components/` will be added in your specific project folder. Create a project folder `src/components/hello_world/`. When creating WebUI resources, follow the Web Development Style Guide. For a sample WebUI page you could start with the following files:

`src/components/hello_world/hello_world.html`
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
`src/components/hello_world/hello_world.css`
```css
p {
  white-space: pre-wrap;
}
```
`src/components/hello_world/hello_world.js`
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

`src/components/hello_world/constants.cc`
```c++
+ const char kChromeUIHelloWorldURL[] = "chrome://hello-world/";
...
+ const char kChromeUIHelloWorldHost[] = "hello-world";
```

`src/components/hello_world/constants.h`
```c++
+ extern const char kChromeUIHelloWorldURL[];
...
+ extern const char kChromeUIHelloWorldHost[];
```


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
