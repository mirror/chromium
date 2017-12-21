// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Set up string assets.
loadTimeData.data = {
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads'
};

// A providing extension which has mounted a file system, and doesn't support
// multiple mounts.
var MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'mounted-single-provider-id',
  iconUrl: 'chrome://mounted-single-extension-id.jpg',
  largeIconUrl: 'chrome://mounted-single-extension-id-200.jpg',
  name: 'mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};

// A providing extension which has not mounted a file system, and doesn't
// support  multiple mounts.
var NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'not-mounted-single-provider-id',
  iconUrl: 'chrome://not-mounted-single-extension-id.jpg',
  largeIconUrl: 'chrome://not-mounted-single-extension-id-200.jpg',
  name: 'not-mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};
// A providing extension which has not mounted a file system, and doesn't
// support  multiple mounts.
var NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'not-mounted-single-provider-id',
  iconUrl: 'chrome://not-mounted-single-extension-id.jpg',
  largeIconUrl: 'chrome://not-mounted-single-extension-id-200.jpg',
  name: 'not-mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};

// A providing extension which has mounted a file system and supports mounting
// more.
var MOUNTED_MULTIPLE_PROVIDING_EXTENSION = {
  providerId: 'mounted-multiple-provider-id',
  iconUrl: 'chrome://mounted-multiple-extension-id.jpg',
  largeIconUrl: 'chrome://mounted-multiple-extension-id-200.jpg',
  name: 'mounted-multiple-extension-name',
  configurable: true,
  watchable: false,
  multipleMounts: true,
  source: 'network'
};

// A providing extension which has not mounted a file system but it's of "file"
// source. Such providers do mounting via file handlers.
var NOT_MOUNTED_FILE_PROVIDING_EXTENSION = {
  providerId: 'file-provider-id',
  iconUrl: 'chrome://file-extension-id.jpg',
  largeIconUrl: 'chrome://file-extension-id-200.jpg',
  name: 'file-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: true,
  source: 'file'
};

// A providing extension which has not mounted a file system but it's of
// "device" source. Such providers are not mounted by user, but automatically
// when the device is attached.
var NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION = {
  providerId: 'device-provider-id',
  iconUrl: 'chrome://device-extension-id.jpg',
  largeIconUrl: 'chrome://device-extension-id-200.jpg',
  name: 'device-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: true,
  source: 'device'
};

var volumeManager = null;

function addProvidedVolume(volumeManager, providerId, volumeId) {
  var fileSystem = new MockFileSystem(volumeId, 'filesystem:' + volumeId);
  fileSystem.entries['/'] = new MockDirectoryEntry(fileSystem, '');

  var volumeInfo = new VolumeInfoImpl(
      VolumeManagerCommon.VolumeType.PROVIDED, volumeId, fileSystem,
      '',                                         // error
      '',                                         // deviceType
      '',                                         // devicePath
      false,                                      // isReadonly
      false,                                      // isReadonlyRemovableDevice
      {isCurrentProfile: true, displayName: ''},  // profile
      '',                                         // label
      providerId,                                 // providerId
      false);                                     // hasMedia

  volumeManager.volumeInfoList.push(volumeInfo);
}

function setUp() {
  // Create a dummy API for fetching a list of providers.
  // TODO(mtomasz): Add some native (non-extension) providers.
  chrome.fileManagerPrivate = {
    getProviders: function(callback) {
      callback([MOUNTED_SINGLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION,
                MOUNTED_MULTIPLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_FILE_PROVIDING_EXTENSION,
                NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION]);
    }
  };
  new MockCommandLinePrivate();
  chrome.runtime = {};

  // Create a dummy volume manager.
  volumeManager = new MockVolumeManagerWrapper();
  addProvidedVolume(
      volumeManager, MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId, 'volume-1');
  addProvidedVolume(
      volumeManager, MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
      'volume-2');
}

function testGetInstalledProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(
      model.getInstalledProviders().then(function(providers) {
        assertEquals(5, providers.length);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[0].providerId);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.iconUrl, providers[0].iconUrl);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.largeIconUrl,
            providers[0].largeIconUrl);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.name, providers[0].name);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.configurable,
            providers[0].configurable);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.watchable,
            providers[0].watchable);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.multipleMounts,
            providers[0].multipleMounts);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.source, providers[0].source);

        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[1].providerId);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
            providers[2].providerId);
        assertEquals(
            NOT_MOUNTED_FILE_PROVIDING_EXTENSION.providerId,
            providers[3].providerId);
        assertEquals(
            NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION.providerId,
            providers[4].providerId);

        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.iconUrl,
            providers[1].iconUrl);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.iconUrl, providers[2].iconUrl);
        assertEquals(
            NOT_MOUNTED_FILE_PROVIDING_EXTENSION.iconUrl, providers[3].iconUrl);
        assertEquals(
            NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION.iconUrl,
            providers[4].iconUrl);

        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.largeIconUrl,
            providers[1].largeIconUrl);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.largeIconUrl,
            providers[2].largeIconUrl);
        assertEquals(
            NOT_MOUNTED_FILE_PROVIDING_EXTENSION.largeIconUrl,
            providers[3].largeIconUrl);
        assertEquals(
            NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION.largeIconUrl,
            providers[4].largeIconUrl);
      }),
      callback);
}

function testGetMountableProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(
      model.getMountableProviders().then(function(providers) {
        assertEquals(2, providers.length);
        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[0].providerId);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
            providers[1].providerId);
      }),
      callback);
}
