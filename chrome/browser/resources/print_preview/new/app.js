// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-app',

  properties: {
    /** @private {!print_preview_new.Model} */
    model_: {
      type: Object,
      notify: true,
      value: {
        previewLoading: false,
        previewFailed: false,
        cloudPrintError: '',
        privetExtensionError: '',
        invalidSettings: false,
        destination: {
          id: 'Foo Printer',
          type: print_preview.DestinationType.LOCAL,
          origin: print_preview.DestinationOrigin.LOCAL,
          displayName: 'Foo Printer',
          isRecent: true,
          isOwned: true,
          account: '',
          description: 'SomePrinterBrand 123',
          location: '',
          tags: [],
          cloudID: '',
          extensionId: '',
          extensionName: '',
          capabilities: {
            version: '1.0',
            printer: {
              collate: true,
              copies: true,
              color: {
                option: [
                  {type: 'STANDARD_COLOR', is_default: true},
                  {type: 'STANDARD_MONOCHROME'},
                ],
              },
            }
          },
          connectionStatus: print_preview.DestinationConnectionStatus.ONLINE,
          provisionalType: print_preview.DestinationProvisionalType.NONE,
          isEnterprisePrinter: false,
          lastAccessTime: 0,
        },
        copies: 1,
        pageRange: [1, 2, 3, 4, 5],
        duplex: false,
        copiesInvalid: false,
        scalingInvalid: false,
        pagesInvalid: false,
        isPdfDocument: true,
        fitToPageScaling: '94',
        documentNumPages: 5,
      },
    },
  }
});
