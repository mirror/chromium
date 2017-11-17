// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');

/**
 * @typedef {{
 *   id: string,
 *   type: !print_preview.DestinationType,
 *   origin: !print_preview.DestinationOrigin,
 *   displayName: string,
 *   isRecent: boolean,
 *   isOwned: boolean,
 *   account: string,
 *   description: string,
 *   location: string,
 *   tags: !Array<string>,
 *   cloudID: string,
 *   extensionId: string,
 *   extensionName: string,
 *   capabilities: ?print_preview.Cdd,
 *   connectionStatus: !print_preview.DestinationConnectionStatus,
 *   provisionalType: !print_preview.DestinationProvisionalType,
 *   isEnterPrisePrinter: boolean,
 *   lastAccessTime: number,
 * }}
 */
print_preview_new.Destination;

/**
 * @param {!print_preview_new.Destination} destination The destination to
 *     evaluate.
 * @return {boolean} Whether the destination is one of the local types
 */
print_preview_new.DestinationIsLocal = function(destination) {
  return destination.origin == print_preview.DestinationOrigin.LOCAL ||
      destination.origin == print_preview.DestinationOrigin.EXTENSION ||
      destination.origin == print_preview.DestinationOrigin.CROS ||
      (destination.origin == print_preview.DestinationOrigin.PRIVET &&
       destination.connectionStatus !=
           print_preview.DestinationConnectionStatus.UNREGISTERED);
};
