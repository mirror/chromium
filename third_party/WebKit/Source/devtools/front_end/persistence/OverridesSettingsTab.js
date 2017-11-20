// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Persistence.OverridesSettingsTab = class extends UI.VBox {
  constructor() {
    super();
    this.registerRequiredCSS('persistence/overridesSettingsTab.css');

    var header = this.element.createChild('header');
    header.createChild('h3').createTextChild(Common.UIString('Overrides'));

    this.containerElement = this.element.createChild('div', 'overrides-settings-container');

    Persistence.isolatedFileSystemManager.addEventListener(
        Persistence.IsolatedFileSystemManager.Events.FileSystemAdded,
        event => this._fileSystemAdded(/** @type {!Persistence.IsolatedFileSystem} */ (event.data)), this);
    Persistence.isolatedFileSystemManager.addEventListener(
        Persistence.IsolatedFileSystemManager.Events.FileSystemRemoved,
        event => this._fileSystemRemoved(/** @type {!Persistence.IsolatedFileSystem} */ (event.data)), this);
    Persistence.networkPersistenceManager.addEventListener(
        Persistence.NetworkPersistenceManager.Events.ProjectDomainChanged, event => {
          var project = /** @type {!Persistence.FileSystemWorkspaceBinding.FileSystem} */ (event.data);
          var fileSystem = Persistence.isolatedFileSystemManager.fileSystem(project.fileSystemPath());
          if (!fileSystem)
            return;
          this._fileSystemRemoved(fileSystem);
          this._fileSystemAdded(fileSystem);
        });

    this._fileSystemsListContainer = this.containerElement.createChild('div', '');

    /** @type {!Map<string, !Element>} */
    this._elementByPath = new Map();

    /** @type {!Map<string, !Persistence.EditFileSystemView>} */
    this._mappingViewByPath = new Map();

    var fileSystems = Persistence.isolatedFileSystemManager.fileSystems();
    for (var i = 0; i < fileSystems.length; ++i) {
      var project = Persistence.networkPersistenceManager.projectForFileSystem(fileSystems[i]);
      if (!project)
        continue;
      this._addItem(project);
    }
  }

  /**
   * @param {!Persistence.FileSystemWorkspaceBinding.FileSystem} project
   */
  _addItem(project) {
    var element = this._renderFileSystem(project);
    this._elementByPath.set(project.fileSystemPath(), element);

    this._fileSystemsListContainer.appendChild(element);

    var mappingView = new Persistence.EditFileSystemView(project.fileSystemPath());
    this._mappingViewByPath.set(project.fileSystemPath(), mappingView);
    mappingView.element.classList.add('file-system-mapping-view');
    mappingView.show(element);
  }

  /**
   * @param {!Persistence.FileSystemWorkspaceBinding.FileSystem} project
   * @return {!Element}
   */
  _renderFileSystem(project) {
    var element = createElementWithClass('div', 'file-system-container');
    var header = element.createChild('div', 'file-system-header');

    header.createChild('div', 'file-system-name').textContent =
        Persistence.networkPersistenceManager.domainForProject(project);
    var path = header.createChild('div', 'file-system-path');
    var fileSystemPath = project.fileSystemPath();
    path.textContent = fileSystemPath;
    path.title = fileSystemPath;

    var toolbar = new UI.Toolbar('');
    var button = new UI.ToolbarButton(Common.UIString('Remove'), 'largeicon-delete');
    var fileSystem = /** @type {!Persistence.IsolatedFileSystem} */ (
        Persistence.isolatedFileSystemManager.fileSystem(fileSystemPath));
    button.addEventListener(UI.ToolbarButton.Events.Click, this._removeFileSystemClicked.bind(this, fileSystem));
    toolbar.appendToolbarItem(button);
    header.appendChild(toolbar.element);

    return element;
  }

  /**
   * @param {!Persistence.IsolatedFileSystem} fileSystem
   */
  _removeFileSystemClicked(fileSystem) {
    Persistence.isolatedFileSystemManager.removeFileSystem(fileSystem);
  }

  /**
   * @param {!Persistence.IsolatedFileSystem} fileSystem
   */
  _fileSystemAdded(fileSystem) {
    var project = Persistence.networkPersistenceManager.projectForFileSystem(fileSystem);
    if (!project)
      return;
    this._addItem(project);
  }

  /**
   * @param {!Persistence.IsolatedFileSystem} fileSystem
   */
  _fileSystemRemoved(fileSystem) {
    var mappingView = this._mappingViewByPath.get(fileSystem.path());
    if (mappingView) {
      mappingView.dispose();
      this._mappingViewByPath.delete(fileSystem.path());
    }

    var element = this._elementByPath.get(fileSystem.path());
    if (element) {
      this._elementByPath.delete(fileSystem.path());
      element.remove();
    }
  }
};
