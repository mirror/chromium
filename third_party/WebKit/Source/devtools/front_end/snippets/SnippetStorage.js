/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @unrestricted
 */
Snippets.SnippetStorage = class extends Common.Object {
  constructor(settingPrefix, namePrefix) {
    super();
    /** @type {!Map<string,!Snippets.Snippet>} */
    this._snippets = new Map();

    this._lastSnippetIdentifierSetting = Common.settings.createSetting(settingPrefix + 'Snippets_lastIdentifier', 0);
    this._snippetsSetting = Common.settings.createSetting(settingPrefix + 'Snippets', []);
    this._namePrefix = namePrefix;

    this._loadSettings();
  }

  get namePrefix() {
    return this._namePrefix;
  }

  _saveSettings() {
    var savedSnippets = [];
    for (var snippet of this._snippets.values())
      savedSnippets.push(snippet.serializeToObject());
    this._snippetsSetting.set(savedSnippets);
  }

  /**
   * @return {!Array<!Snippets.Snippet>}
   */
  snippets() {
    return this._snippets.valuesArray();
  }

  /**
   * @param {string} id
   * @return {?Snippets.Snippet}
   */
  snippetForId(id) {
    return this._snippets.get(id);
  }

  /**
   * @param {string} name
   * @return {?Snippets.Snippet}
   */
  snippetForName(name) {
    for (var snippet of this._snippets.values()) {
      if (snippet.name === name)
        return snippet;
    }
    return null;
  }

  _loadSettings() {
    var savedSnippets = this._snippetsSetting.get();
    for (var i = 0; i < savedSnippets.length; ++i)
      this._snippetAdded(Snippets.Snippet.fromObject(this, savedSnippets[i]));
  }

  /**
   * @param {!Snippets.Snippet} snippet
   */
  deleteSnippet(snippet) {
    this._snippets.delete(snippet.id);
    this._saveSettings();
  }

  /**
   * @return {!Snippets.Snippet}
   */
  createSnippet() {
    var nextId = this._lastSnippetIdentifierSetting.get() + 1;
    var snippetId = String(nextId);
    this._lastSnippetIdentifierSetting.set(nextId);
    var snippet = new Snippets.Snippet(this, snippetId);
    this._snippetAdded(snippet);
    this._saveSettings();
    return snippet;
  }

  /**
   * @param {!Snippets.Snippet} snippet
   */
  _snippetAdded(snippet) {
    this._snippets.set(snippet.id, snippet);
  }
};

/**
 * @unrestricted
 */
Snippets.Snippet = class extends Common.Object {
  /**
   * @param {!Snippets.SnippetStorage} storage
   * @param {string} id
   * @param {string=} name
   * @param {string=} content
   */
  constructor(storage, id, name, content) {
    super();
    this._storage = storage;
    this._id = id;
    this._name = name || storage.namePrefix + id;
    this._content = content || '';
    this._registeredInterceptor = this._onInterceptRequest.bind(this);
    /** @type {?Promise<?SDK.RemoteObject>} */
    this._remoteHandlerPromise = null;
    /** @type {?SDK.ExecutionContext} */
    this._lastContext = null;
    this._registerInterceptor();
  }

  _registerInterceptor() {
    if (this._registeredInterceptor)
      RequestInterceptor.requestInterceptor.unregisterInterceptor(this._registeredInterceptor);
    RequestInterceptor.requestInterceptor.registerInterceptor(this._name, this._registeredInterceptor);
    RequestInterceptor.requestInterceptor.setEnabled(true);
  }

  /**
   * @return {!SDK.ExecutionContext}
   */
  _executionContext() {
    var target = SDK.targetManager.mainTarget();
    if (!target)
      return;
    var runtimeModel = target.model(SDK.RuntimeModel);
    if (!runtimeModel)
      return;
    return runtimeModel.defaultExecutionContext();
  }

  /**
   * @param {!SDK.ExecutionContext} executionContext
   * @return {!Promise<?SDK.RemoteObject>}
   */
  _handlerRemoteObject(executionContext) {
    if (this._remoteHandlerPromise && executionContext === this._lastContext)
      return this._remoteHandlerPromise;
    this._lastContext = executionContext;
    this._remoteHandlerPromise = new Promise(resolve => {
      executionContext.evaluate(
          `(${this.content})`, 'request_interceptor', false, true, false, false, false, (remoteObject, exceptionDetails) => {
            if (exceptionDetails) {
              if (remoteObject)
                remoteObject.release();
              console.error(remoteObject, exceptionDetails);
              remoteObject = null;
            }
            resolve(remoteObject);
          });
    });
    return this._remoteHandlerPromise;
  }

  async _releaseHandlerRemoteObjectIfNeeded() {
    if (!this._remoteHandlerPromise)
      return;
    var remoteObject = await this._remoteHandlerPromise;
    if (!remoteObject)
      return;
    remoteObject.release();
  }

  /**
   * @param {!RequestInterceptor.SharedInterceptionResponse} interceptionResponse
   * @return {!Promise}
   */
  async _onInterceptRequest(interceptionResponse) {
    if (interceptionResponse.headers['x-devtools-bypass-intercept'] === '1') {
      //interceptionResponse.originUrl = 'http://yahoo.com';
      //console.log(interceptionResponse.url, interceptionResponse);
      return;
    }
    //console.log(interceptionResponse.url, interceptionResponse);
    var requestHeaders = interceptionResponse.headers['Access-Control-Request-Headers'];
    if (interceptionResponse.method === 'OPTIONS' && requestHeaders) {
      var headerKeys = new Set(requestHeaders.split(/\*,\s*/));
      if (headerKeys.has('x-devtools-bypass-intercept')) {
        var responseHeaders = ['HTTP/1.1 200 OK'];
        responseHeaders.push('Date: ' + ((new Date()).toUTCString()));
        responseHeaders.push('Access-Control-Allow-Origin: *');
        responseHeaders.push('Access-Control-Allow-Methods: *');
        responseHeaders.push('Access-Control-Allow-Headers: x-devtools-bypass-intercept');
        responseHeaders.push('Access-Control-Max-Age: 86400');
        responseHeaders.push('Content-Length: 0');
        responseHeaders.push('Connection: closed');
        responseHeaders.push('Content-Type: text/plain');
        responseHeaders.push('');
        responseHeaders.push('');
        interceptionResponse.rawResponse = responseHeaders.join('\r\n').toBase64();
        //console.log([responseHeaders.join('\r\n')]);
        return;
      }
    }
    var activeExecutionContext = this._executionContext();
    if (!activeExecutionContext)
      return;
    var remoteObject = await this._handlerRemoteObject(activeExecutionContext);
    if (!remoteObject)
      return;

    var requestRemoteObject = await this._requestRemoteObject(activeExecutionContext, interceptionResponse);
    if (!requestRemoteObject)
      return;
    var result = await remoteObject.callFunctionJSONPromise(
        /**
         * @param {!Request} request
         * @return {!Promise<?{url: string, rawResponse: string}|string>}
         */
        async function (request) {
          try {
            var response = await this.call(null, innerFetch, request);
          } catch (e) {
            console.log(request, e, 'FAILED');
            return 'Failed';
          }
          if (!response)
            return null;
          if (typeof response === 'string')
            return response;
          if (!response instanceof Response) {
            console.error("result of request interception script must be an instance of Request");
            return null;
          }
          var body = ['HTTP/1.1 ' + response.status + ' ' + response.statusText];
          for (var [key, value] of response.headers.entries()) {
            // TODO(allada) This is not safe for new line and such characters.
            body.push(key + ': ' + value);
          }
          body.push('');
          body.push(await response.text() || '');
          //console.log(request, response, [body]);
          return {
            url: response.url,
            rawResponse: body.join("\r\n")
          };

          function innerFetch(input, init) {
            init = init || {};
            init.headers = new Headers(init.headers);
            init.headers.append('x-devtools-bypass-intercept', '1');
            init.mode = 'cors';
            return fetch(input, init);
          }
        }, [SDK.RemoteObject.toCallArgument(requestRemoteObject)], true);

    //console.log(result);
    if (result === null)
      return;
    if (typeof result === 'string')
      interceptionResponse.errorReason = result;
    if (result.url !== undefined)
      interceptionResponse.url = result.url;
    if (result.rawResponse)
      interceptionResponse.rawResponse = result.rawResponse.toBase64();

    // console.log(interceptionResponse);
    // console.log(this.content);
  }

  /**
   * @param {!SDK.ExecutionContext} executionContext
   * @param {!RequestInterceptor.SharedInterceptionResponse} iResponse
   * @return {!Promise<?SDK.RemoteObject}>}
   */
  _requestRemoteObject(executionContext, iResponse) {
    var code = `(new Request('${iResponse.url}', {
        method: '${iResponse.method}',
        headers: new Headers(${JSON.stringify(iResponse.headers)}),
    ` +
        (iResponse.postData ? `    body: new Blob([${JSON.stringify(iResponse.postData)}])\n` : '') + 
    `}))`;
    return new Promise(resolve => {
      executionContext.evaluate(code, 'request_interceptor', false, true, false, false, false, (remoteObject, exceptionDetails) => {
        if (exceptionDetails) {
          console.log(code);
          if (remoteObject)
            remoteObject.release();
          console.error(remoteObject, exceptionDetails);
          remoteObject = null;
        }
        resolve(remoteObject);
      });
    });
  }

  /**
   * @param {!Snippets.SnippetStorage} storage
   * @param {!Object} serializedSnippet
   * @return {!Snippets.Snippet}
   */
  static fromObject(storage, serializedSnippet) {
    return new Snippets.Snippet(storage, serializedSnippet.id, serializedSnippet.name, serializedSnippet.content);
  }

  /**
   * @return {string}
   */
  get id() {
    return this._id;
  }

  /**
   * @return {string}
   */
  get name() {
    return this._name;
  }

  /**
   * @param {string} name
   */
  set name(name) {
    if (this._name === name)
      return;

    this._name = name;
    this._storage._saveSettings();
    this._registerInterceptor();
  }

  /**
   * @return {string}
   */
  get content() {
    return this._content;
  }

  /**
   * @param {string} content
   */
  set content(content) {
    if (this._content === content)
      return;
    this._releaseHandlerRemoteObjectIfNeeded();
    this._content = content;
    this._storage._saveSettings();
  }

  /**
   * @return {!Object}
   */
  serializeToObject() {
    var serializedSnippet = {};
    serializedSnippet.id = this.id;
    serializedSnippet.name = this.name;
    serializedSnippet.content = this.content;
    return serializedSnippet;
  }
};
