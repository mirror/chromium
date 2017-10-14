// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Persistence.NetworkPersistenceManager = class extends Common.Object {
  /**
   * @param {!Workspace.Workspace} workspace
   */
  constructor(workspace) {
    super();
    this._bindingSymbol = Symbol('NetworkPersistenceBinding');
    this._domainPathForProjectSymbol = Symbol('NetworkPersistenceDomainPath');

    this._enabled = false;
    this._workspace = workspace;
    this._domainPathForFileSystemPathSetting = Common.settings.createSetting('domainPathForFileSystemPath', {});
    /** @type {!Set<!Workspace.Project>} */
    this._projects = new Set();
    /** @type {!Map<string, !Workspace.UISourceCode>} */
    this._fileSystemUISourceCodeForUrlMap = new Map();
    this._bindingsThrottler = new Common.Throttler(100);
    this._interceptionHandlerBound = this._interceptionHandler.bind(this);

    this._workspace.addEventListener(
        Workspace.Workspace.Events.ProjectAdded,
        event => this._onProjectAdded(/** @type {!Workspace.Project} */ (event.data)));
    this._workspace.addEventListener(
        Workspace.Workspace.Events.ProjectRemoved,
        event => this._onProjectRemoved(/** @type {!Workspace.Project} */ (event.data)));

    Workspace.workspace.addEventListener(
        Workspace.Workspace.Events.UISourceCodeAdded,
        event => this._onUISourceCodeAdded(/** @type {!Workspace.UISourceCode} */ (event.data)), this);
    Workspace.workspace.addEventListener(
        Workspace.Workspace.Events.UISourceCodeRemoved,
        event => this._onUISourceCodeRemoved(/** @type {!Workspace.UISourceCode} */ (event.data)), this);

    Workspace.workspace.addEventListener(
        Workspace.Workspace.Events.WorkingCopyCommitted,
        event => this._onUISourceCodeWorkingCopyCommitted(
            /** @type {!Workspace.UISourceCode} */ (event.data.uiSourceCode),
            /** @type {string} */ (event.data.content)),
        this);
  }

  /**
   * @param {!Workspace.Project} project
   */
  removeFileSystemProject(project) {
    var fileSystemPath = Persistence.FileSystemWorkspaceBinding.fileSystemPath(project.id());
    var domainPathForFileSystemObject = this._domainPathForFileSystemPathSetting.get();
    delete domainPathForFileSystemObject[fileSystemPath];
    this._domainPathForFileSystemPathSetting.set(domainPathForFileSystemObject);

    for (var uiSourceCode of project.uiSourceCodes())
      this._onUISourceCodeRemoved(uiSourceCode);
    this._onProjectRemoved(project);
  }

  /**
   * @param {string} domainPath
   * @param {!Workspace.Project} project
   */
  addFileSystemProject(domainPath, project) {
    if (this._projects.has(project))
      return;
    var fileSystemPath = Persistence.FileSystemWorkspaceBinding.fileSystemPath(project.id());
    var domainPathForFileSystemObject = this._domainPathForFileSystemPathSetting.get();
    domainPathForFileSystemObject[fileSystemPath] = domainPath;
    this._domainPathForFileSystemPathSetting.set(domainPathForFileSystemObject);

    this._onProjectAdded(project);
    for (var uiSourceCode of project.uiSourceCodes())
      this._onUISourceCodeAdded(uiSourceCode);
  }

  /**
   * @return {!Set<!Workspace.Project>}
   */
  projects() {
    return this._projects;
  }

  /**
   * @param {!Workspace.Project} project
   * @return {?string}
   */
  domainPathForProject(project) {
    return project[this._domainPathForProjectSymbol] || null;
  }

  /**
   * @param {boolean} enabled
   */
  setEnableInterception(enabled) {
    if (this._enabled === enabled)
      return;
    this._enabled = enabled;
    for (var project of this._projects) {
      for (var uiSourceCode of project.uiSourceCodes()) {
        if (enabled)
          this._onUISourceCodeAdded(uiSourceCode);
        else
          this._onUISourceCodeRemoved(uiSourceCode);
      }
    }
  }

  /**
   * @param {!Workspace.UISourceCode} fileSystemUISourceCode
   * @return {!Array<!Workspace.UISourceCode>}
   */
  _networkUISourceCodes(fileSystemUISourceCode) {
    var networkUISourceCodes = [];
    var networkProjects = this._workspace.projectsForType(Workspace.projectTypes.Network);
    var urls = this._urlsForFileSystemUISourceCode(fileSystemUISourceCode);
    for (var networkProject of networkProjects) {
      for (var url of urls) {
        var networkUISourceCode = networkProject.uiSourceCodeForURL(url);
        if (!networkUISourceCode)
          continue;
        networkUISourceCodes.push(networkUISourceCode);
      }
    }
    return networkUISourceCodes;
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _removeNetworkBinding(uiSourceCode) {
    var binding = uiSourceCode[this._bindingSymbol];
    if (!binding)
      return;
    delete binding.network[this._bindingSymbol];
    Persistence.persistence.mapping().removeBinding(binding);
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _removeFileSystemBinding(uiSourceCode) {
    if (!this._projects.has(uiSourceCode.project()))
      return;
    for (var networkUISourceCode of this._networkUISourceCodes(uiSourceCode)) {
      var binding = networkUISourceCode[this._bindingSymbol];
      if (!binding)
        continue;
      this._removeNetworkBinding(networkUISourceCode);
    }
  }

  /**
   * @param {!Workspace.UISourceCode} networkUISourceCode
   * @param {!Workspace.UISourceCode} fileSystemUISourceCode
   */
  _bind(networkUISourceCode, fileSystemUISourceCode) {
    var binding = /** @type {!Persistence.PersistenceBinding|undefined} */ (networkUISourceCode[this._bindingSymbol]);
    if (binding && binding.fileSystem === fileSystemUISourceCode)
      return;
    if (binding)
      this._removeNetworkBinding(binding.network);
    binding = new Persistence.PersistenceBinding(networkUISourceCode, fileSystemUISourceCode, true);
    networkUISourceCode[this._bindingSymbol] = binding;
    Persistence.persistence.mapping().addBinding(binding);
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   * @param {string} content
   */
  _onUISourceCodeWorkingCopyCommitted(uiSourceCode, content) {
    if (!this._enabled || uiSourceCode.project().type() !== Workspace.projectTypes.Network ||
        uiSourceCode[this._bindingSymbol])
      return;
    for (var project of this._projects) {
      var projectDomainPath = project[this._domainPathForProjectSymbol];
      var urlDomainPath = uiSourceCode.url().replace(/^https?:\/\//, '');
      if (!urlDomainPath.startsWith(projectDomainPath))
        return;
      var relativeFilePath = urlDomainPath.substr(projectDomainPath.length);
      var fileName = relativeFilePath.substr(relativeFilePath.lastIndexOf('/') + 1);
      var relativeFolderPath = relativeFilePath.substr(0, relativeFilePath.length - fileName.length);
      project.createFile(relativeFolderPath, fileName, content, true);
    }
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   * @return {!Array<string>}
   */
  _urlsForFileSystemUISourceCode(uiSourceCode) {
    var domainPath = this.domainPathForProject(uiSourceCode.project());
    var directoryPath = Persistence.FileSystemWorkspaceBinding.fileSystemPath(uiSourceCode.project().id());
    var relativePath = uiSourceCode.url().substr(directoryPath.length + 1);
    return ['http://' + domainPath + relativePath, 'https://' + domainPath + relativePath];
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _onUISourceCodeAdded(uiSourceCode) {
    if (!this._enabled)
      return;
    if (uiSourceCode.project().type() === Workspace.projectTypes.Network) {
      if (uiSourceCode[this._bindingSymbol])
        return;
      var fileSystemUISourceCode = this._fileSystemUISourceCodeForUrlMap.get(uiSourceCode.url());
      if (!fileSystemUISourceCode)
        return;
      this._bind(uiSourceCode, fileSystemUISourceCode);
      return;
    }
    if (!this._projects.has(uiSourceCode.project()))
      return;
    var urls = this._urlsForFileSystemUISourceCode(uiSourceCode);
    for (var url of urls)
      this._fileSystemUISourceCodeForUrlMap.set(url, uiSourceCode);
    SDK.multitargetNetworkManager.setInterceptionHandlerForPatterns(
        this._interceptionHandlerBound, Array.from(this._fileSystemUISourceCodeForUrlMap.keys()));

    for (var networkUISourceCode of this._networkUISourceCodes(uiSourceCode))
      this._bind(networkUISourceCode, uiSourceCode);
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _onUISourceCodeRemoved(uiSourceCode) {
    if (uiSourceCode.project().type() === Workspace.projectTypes.Network) {
      this._removeNetworkBinding(uiSourceCode);
      return;
    }
    if (!this._projects.has(uiSourceCode.project()))
      return;

    for (var url of this._urlsForFileSystemUISourceCode(uiSourceCode))
      this._fileSystemUISourceCodeForUrlMap.delete(url);
    SDK.multitargetNetworkManager.setInterceptionHandlerForPatterns(
        this._interceptionHandlerBound, Array.from(this._fileSystemUISourceCodeForUrlMap.keys()));
    this._removeFileSystemBinding(uiSourceCode);
  }

  /**
   * @param {!Workspace.Project} project
   */
  _onProjectAdded(project) {
    if (project.type() !== Workspace.projectTypes.FileSystem)
      return;
    var fileSystemPath = Persistence.FileSystemWorkspaceBinding.fileSystemPath(project.id());
    var localPathForDomainPathObject = this._domainPathForFileSystemPathSetting.get();
    var domainPath = localPathForDomainPathObject[fileSystemPath];
    if (!domainPath)
      return;
    project[this._domainPathForProjectSymbol] = domainPath;
    this._projects.add(project);
    this.dispatchEventToListeners(Persistence.NetworkPersistenceManager.Events.ProjectsChanged);
  }

  /**
   * @param {!Workspace.Project} project
   */
  _onProjectRemoved(project) {
    if (!this._projects.has(project))
      return;
    this._projects.delete(project);
    this.dispatchEventToListeners(Persistence.NetworkPersistenceManager.Events.ProjectsChanged);
  }

  /**
   * @param {!SDK.MultitargetNetworkManager.InterceptedRequest} interceptedRequest
   * @return {!Promise}
   */
  async _interceptionHandler(interceptedRequest) {
    var fileSystemUISourceCode = this._fileSystemUISourceCodeForUrlMap.get(interceptedRequest.request.url);
    if (!fileSystemUISourceCode)
      return;
    var content = await fileSystemUISourceCode.requestContent();
    interceptedRequest.continueRequestWithContent(new Blob([content], {type: fileSystemUISourceCode.mimeType()}));
  }
};

Persistence.NetworkPersistenceManager.Events = {
  ProjectsChanged: Symbol('ProjectsChanged')
};

/** @type {!Persistence.NetworkPersistenceManager} */
Persistence.networkPersistenceManager;
