# Cross-Site Document Blocking (XSDB)

### The problem

The same-origin policy allows a web site to access *documents* (e.g. HTML, XML
and JSON files) from its own site, as well as *resources* (e.g. images, scripts
and stylesheets) from any site.

Cross-Site Document Blocking (XSDB) is a security feature that prevents exposing
*documents* to another site.  XSDB works by blocking the body of *document*
responses from reaching a cross-site page.

XSDB mitigates the following attack vectors:

* Cross-Site Script Inclusion (XSSI).

  * For example, an attacker may 1) poison the Javascript Array constructor with
    attacker-controlled code and and 2) intercept cross-site data by
    constructing JSON arrays via
    `<script src="https://example.com/secret.json">`.
    While the array constructor attack vector is fixed in every major, modern
    browser, researchers are looking for and discovering new attack vectors
    (for example, see the
    [slides here](https://www.owasp.org/images/6/6a/OWASPLondon20161124_JSON_Hijacking_Gareth_Heyes.pdf)
    ).

  * XSDB prevents this class of attacks, by preventing the secret *document*
    from being delivered to a cross-site `<script>` element.  XSDB is
    particularly valuable in absence of other XSSI defenses like XSRF tokens
    and/or JSON parser breakers.

* Speculative Side Channel Attack (e.g. Spectre).

  * For example, an attacker may 1) use an `<img
    src="https://example.com/secret.json">` element to pull a cross-site secret
    into the process where the attacker's Javascript runs, and then 2) use a
    speculative side channel attack (e.g. Spectre) to read the secret.

  * XSDB can prevent this class of attacks when used in tandem with Site
    Isolation, by preventing the secret *document* from being present in the
    memory of a process hosting a cross-site page.


### Goals

* XSDB should block *documents* from reaching cross-site pages, unless
  explicitly permitted by CORS.

* XSDB should protect as many *documents* as possible.

* XSDB should cause minimal web-compatibility breakages.

* XSDB should be resilient to the most frequent Content-Type mislabelings,
  unless the site owner asserts that the response is labelled correctly via
  `X-Content-Type-Options: nosniff` header.


### How does XSDB "block" a response?

XSDB blocks responses by replacing their body with an empty response.
This is consistent with the handling of
[filtered, opaque responses](https://fetch.spec.whatwg.org/#concept-filtered-response-opaque)
by https://fetch.spec.whatwg.org/.

XSDB doesn't affect the headers of a response.

> [lukasza@chromium.org] Should XSDB trim the response headers down
> to https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name ?
> Surely, some headers are worth protecting (even Content-Length and/or
> Content-MD5 can undesirable disclose a small amount of information).
> What are the risks associated with trimming down response headers?

XSDB blocking should take place before the response reaches the process hosting
the cross-origin initiator of the request.  In other words, XSDB blocking should
prevent cross-origin *documents* from ever being present in the memory of the
process hosting a cross-origin website (even temporarily or for a short term).

> [lukasza@chromium.org] In Chrome this means XSDB would block the response in
> the browser process (or the network service process in the future) - before
> the response reaches the renderer process.


### What kinds of requests can XSDB block?

XSDB should not block
[navigation requests](https://fetch.spec.whatwg.org/#navigation-request)
or requests where the
[request destination](https://fetch.spec.whatwg.org/#concept-request-destination)
is "object" or "embed".

> [lukasza@chromium.org] Enforcing isolation between cross-origin `<iframe>`s or
> `<object>`s or is outside the scope of XSDB (and depends on Site Isolation
> approach specific to each browser).

> [lukasza@chromium.org] TODO: Figure out how
> [Edge's VM-based isolation](https://cloudblogs.microsoft.com/microsoftsecure/2017/10/23/making-microsoft-edge-the-most-secure-browser-with-windows-defender-application-guard/)
> works (e.g. if some origins are off-limits in particular renderers, then this
> would greatly increase utility of XSDB in Edge).

> [lukasza@chromium.org] TODO: Figure out how other browsers approach Site
> Isolation (e.g. even if there is no active work, maybe there are some bugs we
> can reference here).

XSDB should not block download requests (e.g. requests where the
[initiator](https://fetch.spec.whatwg.org/#concept-request-initiator)
is "download").

> [lukasza@chromium.org] AFAIK, in Chrome a response to a download request never
> passes through memory of a renderer process.  Not sure if this is true in
> other browsers.

XSDB should block all other kinds of requests including:
* [XHR](https://xhr.spec.whatwg.org/)
  and [fetch()](https://fetch.spec.whatwg.org/#dom-global-fetch)
* `ping`, `navigator.sendBeacon()`
* `<link rel="prefetch" ...>`
* Requests with the following
  [request destination](https://fetch.spec.whatwg.org/#concept-request-destination):
    - "image" requests like `<img>` tag, `/favicon.ico`, SVG's `<image>`,
      CSS' `background-image`, etc.
    - [script-like destinations](https://fetch.spec.whatwg.org/#request-destination-script-like)
      like `<script>`, `importScripts()`, `navigator.serviceWorker.register()`,
      `audioWorkler.addModule()`, etc.
    - "audio", "video" or "track"
    - "font"
    - "style"
    - "report" requests like CSP reports, NEL reports, etc.


### What is a *document*?  What is a *resource*?

XSDB decides whether a response is a *document* or a *resource* primarily based
on the Content-Type header.

XSDB considers responses with the following `Content-Type` headers to be
*resources* (these are the content types that can be used in `<img>`, `<audio>`,
`<video>`, `<script>` and other similar elements which may be embedded
cross-origin):
* [JavaScript MIME type](https://html.spec.whatwg.org/#javascript-mime-type)
  like `application/javascript` or `text/jscript`
* `text/css`
* [image types](https://mimesniff.spec.whatwg.org/#image-type) like types
  matching `image/*`
* [audio or video types](https://mimesniff.spec.whatwg.org/#audio-or-video-type)
  like `audio/*`, `video/*` or `application/ogg`
* [text/vtt](https://w3c.github.io/webvtt/#file-structure)
* `font/*` or one of legacy
  [font types](https://mimesniff.spec.whatwg.org/#font-type)

> [lukasza@chromium.org] Some images (and other content types) may contain
> sensitive data that shouldn't be shared with other origins.  To avoid breaking
> existing websites images have to be treated by default as cross-origin
> *resources*, but maybe we should consider letting websites opt-into additional
> protection.  For examples a server might somehow indicate to treat its images
> as origin-bound *documents* protected by XSDB (e.g.  with a new kind of HTTP
> response header that we might want to consider).

XSDB considers HTML, XML and JSON to be *documents* - this covers responses
with one of the following `Content-Type` headers:
* [HTML MIME type](https://mimesniff.spec.whatwg.org/#html-mime-type)
* [XML MIME type](https://mimesniff.spec.whatwg.org/#xml-mime-type)
  (except `image/svg+xml` which is a *resource*)
* [JSON MIME type](https://html.spec.whatwg.org/#json-mime-type)

Responses marked as `multipart/*` are conservatively considered *resources*.
This avoids having to parse the content types of the nested parts.
We recommend not supporting multipart range requests for sensitive documents.

Responses with a MIME type not explicitly named above (e.g. `application/pdf` or
`application/zip`) are considered to be *documents*.  Similarly, responses that
don't contain a `Content-Type` header, are also considered *documents*.  This
helps meet the goal of protecting as many *documents* as possible.

> [lukasza@chromium.org] Maybe this is too aggressive for the initial launch of
> XSDB?  See also https://crbug.com/802836.  OTOH, it seems that in the
> long-term this is the right approach (e.g. defining a short list of types and
> type patterns that don't need protection, rather than trying to define a long
> and open-ended list of types that need protection today or would need
> protection in the future).

XSDB considers `text/plain` to be a *document*.

> [lukasza@chromium.org] This seems like a one-off in the current
> implementation.  Maybe `text/plain` should just be treated as
> "a MIME type not explicitly named above".

Additionally XSDB may classify as a *document* any response starting with a JSON
parser breaker (e.g. `)]}'` or `{}&&` or `for(;;);`), regardless of its
Content-Type and regardless of the presence of `X-Content-Type-Options: nosniff`
header.  A JSON parser breaker is highly unlikely to be present in a *resource*
(and therefore highly unlikely to lead to misclassification of a *resource* as a
*document*) - for example:

* A JSON parser breaker would cause a syntax error (or a hang) if present
  in an `application/javascript`.
* A JSON parser breaker is highly unlikely at the beginning of binary
  *resources* like images, videos or fonts (which typically require
  the first few bytes to be hardcoded to a specific sequence - for example
  `FF D8 FF` for image/jpeg).

`text/css` Content-Type should take precedence over presence of JSON parser
breaker and result in classifying the response as a *resource*.  This exception
is needed, because it is possible to construct a file that begins with a JSON
parser break, but at the same time parses fine as a stylesheet - for example:
```css
)]}'
{}
h1 { color: red; }
```

> [lukasza@chromium.org] 

### Sniffing for *resources* served with an incorrect MIME type.

XSDB can't always rely solely on the MIME type of the HTTP response to
distinguish documents from resources, since the MIME type on network responses
is sometimes wrong.  For example, some HTTP servers return a JPEG image with
a `Content-Type` header incorrectly saying `text/html`.

To avoid breaking existing websites, XSDB may attempt to confirm if the response
is really a *document* by sniffing the response body:

* XSDB will only sniff to confirm the classification based on the `Content-Type`
  header (i.e. if the `Content-Type` header is `text/json` then XSDB will sniff
  for JSON and will not sniff for HTML and/or XML).

* XSDB will only sniff to confirm if the response body is a HTML, XML or JSON
  *document* (and won't attempt to sniff content of other types of *documents*
  like PDFs and/or ZIP archives).

> [lukasza@chromium.org] It is not practical to try teaching XSDB about sniffing
> all possible types of *documents* like `application/pdf`, `application/zip`,
> etc.

> [lukasza@chromium.org] Some *document* types are inherently not sniffable
> (for example `text/plain`).

XSDB should trust the `Content-Type` header in scenarios where sniffing
shouldn't or cannot be done:

* When `X-Content-Type-Options: nosniff` header is present.

* When the response is a partial, 206 response.

> [lukasza@chromium.org] An alternative behavior would be to allow (instead of
> blocking) 206 responses that would be sniffable otherwise (so one of HTML, XML
> or JSON + not accompanied by a nosniff header).  This alternative behavior
> would decrease the risk of blocking mislabeled resources, but would increase
> the risk of not blocking documents that need protection (an attacker could
> just need to issue a range request - protection in this case would depend on
> whether 1) the response includes a nosniff header and/or 2) the server rejects
> range requests altogether).  Note that the alternative behavior doesn't help
> with mislabeled text/plain responses (see also https://crbug.com/801709).

Sniffing is a best-effort heuristic and for best security results, we recommend
1) marking responses with the correct Content-Type header and 2) opting out of
sniffing by using the `X-Content-Type-Options: nosniff` header.


### What is XSDB compatibility with existing websites?

XSDB has no impact on the following scenarios:

* **Prefetch**
  * XSDB blocks response body from reaching a cross-origin renderer, but
    XSDB doesn't prevent the response body from being cached by the browser
    process (and subsequently delivered into another, same-origin renderer
    process).

* **Tracking and reporting**
  * Various techniques try to check that a user has accessed some content
    by triggering a web request to a HTTP server that logs the user visit.
    The web request is frequently triggered by an invisible `img` element
    to a HTTP URI that usually replies either with a 204 or with a
    short HTML document.  In addition to the `img` tag, websites may use
    `style`, `script` and other tags to track usage.
  * XSDB won't impact these techniques, as they don't depend on the actual
    content being returned by the HTTP server.  This also applies to other
    web features that don't care about the response: beacons, pings, CSP
    violation reports, etc.

* **Service workers**
  * Service workers may intercept cross-origin requests and artificially
    construct a response *within* the service worker (i.e. without crossing
    origins and/or security boundaries).  XSDB will not block such responses.
  * When service workers cache actual cross-origin responses (e.g. in 'no-cors'
    request mode), the responses are 'opaque' and therefore XSDB can block such
    responses without changing the service worker's behavior ('opaque' responses
    have a non-accessible body even without XSDB).

If XSDB can reliably distinguish *documents* from *resources*, then XSDB should
be mostly non-disruptive and effectively transparent to websites.  For example,
`<img src="https://example.com/document.html">` should behave the same when
given an HTML document without XSDB and when given an empty response body in
presence of XSDB.  Still, there are some cases where XSDB can cause observable
changes in web behavior, even when sniffing identifies the response as requiring
protection:

* **HTML response which is also a valid CSS**.
  Some responses may sniff as valid HTML, but might also parse as valid CSS.
  When such responses get blocked by XSDB, it may break websites using them
  as stylesheets.  Example of such a polyglot response:
```css
<!DOCTYPE html>
<style> h2 {}
h1 { color: blue; }
</style>
```

* **Javascript syntax errors**.
  HTML document loaded into a `<script>` element will typically result in syntax
  errors which can be observed via
  [GlobalEventHandlers.onerror](https://developer.mozilla.org/en-US/docs/Web/API/GlobalEventHandlers/onerror)
  event handler.  Since XSDB replaces body of HTML documents with an empty body,
  no syntax error will be reported when XSDB is present.

> [lukasza@chromium.org] Maybe XSDB should instead replace the response body
> with something like "Blocked by XSDB" (which would still trigger a syntax
> error).  Not sure if it is really worth 1) the effort and 2) inconsistency
> with filtered/opaque response from Fetch spec.

XSDB can mistakenly treat a *resource* as if it were a *document* when **both**
of the following conditions take place:

* The Content-Type header incorrectly indicates that the response is a
  *document* because either
  - An incorrect Content-Type header is present and is not one of explicitly
    whitelisted types (not an image/*, application/javascript, multipart/*,
    etc.).
  - The Content-Type header is missing

* XSDB is not able to confirm that the Content-Type matches response body, when
  either:
  - `X-Content-Type-Options: nosniff` header is present.
  - The Content-Type is not one of types sniffable by XSDB (HTML, JSON or XML).
  - The response is a 206 content range responses with a single-range.

> [lukasza@chromium.org] We believe that mislabeling as HTML, JSON or XML is
> most common.  TODO: are we able to back this up with some numbers?

> [lukasza@chromium.org] Note that range requests are typically not issued
> when making requests for scripts and/or stylesheets.

When XSDB mistakenly treats a *resource* as if it were a *document*, then it may
have the following effect observable either by scripts or users (possibly
breaking the website that depends on the mislabeled *resources*):

* Rendering of blocked images, videos, audio may be broken
* Scripts may not execute
* Stylesheets may not apply

> [lukasza@chromium.org] TODO: Try to quantify how many websites might be
> impacted.  This discussion will probably have to take place (or at least
> start) in a Chrome-internal document - AFAIK UMA results should not be
> shared publicly :-/

Note that XSDB is inactive in the following scenarios:

* Requests allowed via CORS
* Requests initiated by content scripts or plugins

> [lukasza@chromium.org] TODO: Do we need to be more explicit about handling of
> requests initiated by plugins?
> - Should XSDB attempt to intercept and parse
>   [crossdomain.xml](https://www.adobe.com/devnet/articles/crossdomain_policy_file_spec.html)
>   which tells Adobe Flash whether a particular cross-origin request is okay or
>   not (similarly to how XSDB needs to understand CORS response headers)?
> - If XSDB doesn't have knowledge about `crossdomain.xml`, then it will be
>   forced to allow all responses to Flash-initiated requests.  We should
>   clarify why XSDB still provides security benefits in this scenario.
> - Also - not sure if plugin behavior is in-scope of
>   https://fetch.spec.whatwg.org?


### Demo page

To test XSDB one can turn on the feature in Chrome M63+ by launching it with the
`--enable-features=CrossSiteDocumentBlockingAlways` cmdline flag.

XSDB demo page: https://anforowicz.github.io/xsdb-demo/index.html


### Summary of XSDB behavior

* Protected origins
  * XSDB SHOULD allow same-origin responses.
  * XSDB SHOULD block cross-origin responses from HTTP and/or HTTPS origins
    (this includes responses from `filesystem:` and `blob:` URIs if their nested
    origin has a HTTP and/or HTTPS scheme).
  * XSDB MAY block cross-origin responses from non-HTTP/HTTPS origins.

> [lukasza@chromium.org] Should the filesystem/blob part be somehow weaved into
> one of explainer sections above?

* Initiator origins
    * XSDB SHOULD block responses for requests initiated from HTTP/HTTPS origins.
    * XSDB SHOULD block responses for requests initiated from
      opaque/unique/sandboxed origins.
    * XSDB MAY allow responses for requests initiated from `file:` origins.
    * XSDB MAY allow responses for requests initiated from content scripts of
      browser extensions.

* Interoperability with other origin-related policies
    * XSDB SHOULD allow responses that are otherwise allowed by CORS
    * XSDB SHOULD allow non-opaque responses from service workers
    * XSDB SHOULD block opaque responses from service workers

* *document*-vs-*resource* classification
    * XSDB SHOULD classify as a *resource* all responses with the following
      Content-Type:
      * `application/javascript`
      * `text/css`
      * [image types](https://mimesniff.spec.whatwg.org/#image-type) - types
        matching `image/*`
      * [audio or video types](https://mimesniff.spec.whatwg.org/#audio-or-video-type)
        like `audio/*`, `video/*` or `application/ogg`
      * `font/*` or one of legacy
        [font types](https://mimesniff.spec.whatwg.org/#font-type)
    * XSDB SHOULD classify as a *document* all responses with the following
      Content-Type if either 1) the response sniffs as the reported type or 2)
      the `X-Content-Type-Options: nosniff` header is present:
      * [HTML MIME type](https://mimesniff.spec.whatwg.org/#html-mime-type)
      * [XML MIME type](https://mimesniff.spec.whatwg.org/#xml-mime-type)
        (except `image/svg+xml` which is a *resource*)
      * JSON MIME type - one of `text/json`, `text/json+*`, `text/x-json`,
        `text/x-json+*`, `application/json`, `application/json+*` or `*+json`
    * XSDB MAY classify as a *document* any response with a Content-Type not
      explicitly listed in the previous 2 bullet items.
    * XSDB MAY classify as a *document* any response with a missing Content-Type
      response header.
    * XSDB MAY classify as a *document* any response starting with a JSON parser
      breaker (e.g. `)]}'` or `{}&&` or `for(;;);`), regardless of its
      Content-Type and regardless of the presence of `X-Content-Type-Options:
      nosniff` header.

> [lukasza@chromium.org] Should the JSON parser breaker sniffing be somehow
> weaved into one of explainer sections above?

* Sniffing to confirm the Content-Type of the response
    * XSDB SHOULD NOT sniff if `X-Content-Type-Options: nosniff` is present.
    * XSDB MAY avoid sniffing 206 content range responses with a single-range.
    * XSDB MAY limit sniffing to the first few network packets.
    * If Content-Type is `text/html` then XSDB SHOULD allow the response
      if it doesn't
      [sniff as `text/html`](https://mimesniff.spec.whatwg.org/#rules-for-identifying-an-unknown-mime-type).
    * If Content-Type is one of TODO then XSDB SHOULD allow the response
      if it doesn't
      [sniff as `text/xml`](https://mimesniff.spec.whatwg.org/#rules-for-identifying-an-unknown-mime-type).
    * If Content-Type is one of TODO then XSDB SHOULD allow the response
      if it doesn't sniff as JSON.  TODO: define "sniff as JSON".


### Related specs

* https://fetch.spec.whatwg.org/#concept-filtered-response-opaque
* https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name
* https://fetch.spec.whatwg.org/#http-cors-protocol
* https://fetch.spec.whatwg.org/#should-response-to-request-be-blocked-due-to-nosniff
* https://fetch.spec.whatwg.org/#x-content-type-options-header
* https://tools.ietf.org/html/rfc7233#section-4 (Responses to a Range Request)
