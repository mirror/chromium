# Cross-Site Document Blocking (XSDB)

### The problem

The same-origin policy allows a web site to request *documents* (e.g. HTML, XML
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

  * XSDB prevents this class of attacks, by preventing the secret *document*
    from being delivered to a cross-site `<script>` element.  XSDB is
    particularily valuable in absence of other XSSI defenses like XSRF tokens
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

* Block *documents* from reaching cross-site pages.
* Protect as many *documents* as possible,
* Avoid breaking existing websites (e.g. by unintentionally blocking
  *resources* they depend on).


### How does XSDB "block" a response?

XSDB blocks responses by replacing their body with an empty response.

XSDB doesn't affect the headers of a response.

> [lukasza@chromium.org] Should XSDB trim the response headers down
> to https://fetch.spec.whatwg.org/#cors-safelisted-request-header ?


### What is a *document*?  What is a *resource*?

XSDB decides whether a response is a *document* or a *resource* primarily based
on the Content-Type header.

Responses marked as `application/javascript`, `text/css`, `audio/*`, `video/*`,
`font/*`, `image/*` are considered *resources* (these are the content types that
can be used in `<img>`, `<audio>`, `<video>`, `<script>` and other similar
elements that are not subject to the same-origin policy).

XSDB considers HTML, XML and JSON to be *documents* - this covers responses
marked as `text/html`, `text/xml`, `text/xml+*`, `application/xml`,
`application/xml+*`, `*+xml` (except `image/svg+xml` which matches `image/*` and
is therefore a *resource*), `text/json`, `text/json+*`, `text/x-json`,
`text/x-json+*`, `application/json`, `application/json+*`, `*+json`.

Responses marked as `multipart/*` are conservatively considered *resources*.
This avoids having to parse the content types of the nested parts.
We recommend not supporting multipart range requests for sensitive documents.

Response with a mime type not explicitly named above (e.g. `application/pdf`)
are considered to be *documents*.  Similariyly, response that don't contain a
`Content-Type` header, are also considered *documents*.  This helps meet the
goal of protecting as many *documents* as possible.

> [lukasza@chromium.org] Is this too aggressive for the initial launch of XSDB?
> See also https://crbug.com/802836.

XSDB considers `text/plain` to be a *document*.

> [lukasza@chromium.org] This seems like a one-off in the current
> implementation.  Maybe `text/plain` should just be treated as
> "a mime type not explicitly named above".


### Sniffing for *resources* served with an incorrect mime type.

XSDB can't always rely solely on the MIME type of the HTTP response to
distinguish documents from resources, since the MIME type on network responses
is frequently wrong.  For example - a HTTP server may return an JPEG image with
a `Content-Type` header incorrectly saying `text/html`.

To avoid breaking existing websites, XSDB may attempt to confirm if the response
is really a *document* by sniffing the response body:

* XSDB will only sniff to confirm the classification based on the `Content-Type`
  header (i.e. if the `Content-Type` header is `text/json` then XSDB will sniff
  for JSON and will not sniff for HTML and/or XML).
* XSDB will only sniff to confirm if the response body is a HTML, XML or JSON
  *document*.

> [lukasza@chromium.org] It is not practical to teach XSDB about sniffing all
> possible types of *documents* like `application/pdf`, `application/msword`,
> etc.

> [lukasza@chromium.org] Some *document* types are inherently not sniffable
> (for example `text/plain`).

Sniffing is a best-effort heuristic and for best security results, we recommend
1) marking responses with the correct Content-Type header and 2) opting out
of sniffing by using the `X-Content-Type-Options: nosniff` header.


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
      * `audio/*`
      * `video/*`
      * `font/*`
      * `image/*`
    * XSDB SHOULD classify as a *document* all responses with the following
      Content-Type *iff* the response sniffs as matching type:
      * `text/html`
      * `text/xml`
      * `text/xml+*`
      * `application/xml`
      * `application/xml+*`
      * `*+xml` (except `image/svg+xml` which is a *resource*)
      * `text/json`
      * `text/json+*`
      * `text/x-json`
      * `text/x-json+*`
      * `application/json`
      * `application/json+*`
      * `*+json`
    * XSDB MAY classify as a *document* any response with a Content-Type not
      explicitly listed in the previous 2 bullet items.
    * XSDB MAY classify as a *document* any response with a missing Content-Type
      response header.
    * XSDB MAY classify as a *document* any response starting with a JSON parser
      breaker (e.g. `)]}'` or `{}&&` or `for(;;);`), regardless of its
      Content-Type.

* Sniffing to confirm the Content-Type of the response
    * XSDB SHOULD NOT sniff if `X-Content-Type-Options: nosniff` is present.
    * XSDB MAY avoid sniffing 206 content range responses with a single-range.
    * XSDB MAY limit sniffing to the first few network packets.
    * If Content-Type is `text/html` then XSDB SHOULD allow the response
      if it doesn't sniff as HTML.  TODO: define "sniff as HTML".
    * If Content-Type is one of TODO then XSDB SHOULD allow the response
      if it doesn't sniff as XML.  TODO: define "sniff as XML".
    * If Content-Type is one of TODO then XSDB SHOULD allow the response
      if it doesn't sniff as JSON.  TODO: define "sniff as JSON".


### Related specs

* https://fetch.spec.whatwg.org/#concept-filtered-response-opaque
* https://fetch.spec.whatwg.org/#http-cors-protocol
* https://fetch.spec.whatwg.org/#x-content-type-options-header
* https://tools.ietf.org/html/rfc7233#section-4 (Responses to a Range Request)

