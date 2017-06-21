// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.ComponentName;
import android.content.Context;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadNotificationService;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.download.items.OfflineContentAggregatorFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.widget.DateDividedAdapter.TimedItem;
import org.chromium.components.offline_items_collection.OfflineContentProvider;
import org.chromium.components.offline_items_collection.OfflineItem;
import org.chromium.components.offline_items_collection.OfflineItem.Progress;
import org.chromium.components.offline_items_collection.OfflineItemFilter;
import org.chromium.components.offline_items_collection.OfflineItemState;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.ui.widget.Toast;

import java.io.File;
import java.util.Collections;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/** Wraps different classes that contain information about downloads. */
public class DownloadHistoryItemWrapper extends TimedItem {
    public static final Integer FILE_EXTENSION_OTHER = 0;
    public static final Integer FILE_EXTENSION_APK = 1;
    public static final Integer FILE_EXTENSION_CSV = 2;
    public static final Integer FILE_EXTENSION_DOC = 3;
    public static final Integer FILE_EXTENSION_DOCX = 4;
    public static final Integer FILE_EXTENSION_EXE = 5;
    public static final Integer FILE_EXTENSION_PDF = 6;
    public static final Integer FILE_EXTENSION_PPT = 7;
    public static final Integer FILE_EXTENSION_PPTX = 8;
    public static final Integer FILE_EXTENSION_PSD = 9;
    public static final Integer FILE_EXTENSION_RTF = 10;
    public static final Integer FILE_EXTENSION_TXT = 11;
    public static final Integer FILE_EXTENSION_XLS = 12;
    public static final Integer FILE_EXTENSION_XLSX = 13;
    public static final Integer FILE_EXTENSION_ZIP = 14;
    public static final Integer FILE_EXTENSION_BOUNDARY = 15;

    private static final Map<String, Integer> EXTENSIONS_MAP;
    static {
        Map<String, Integer> extensions = new HashMap<>();
        extensions.put("apk", FILE_EXTENSION_APK);
        extensions.put("csv", FILE_EXTENSION_CSV);
        extensions.put("doc", FILE_EXTENSION_DOC);
        extensions.put("docx", FILE_EXTENSION_DOCX);
        extensions.put("exe", FILE_EXTENSION_EXE);
        extensions.put("pdf", FILE_EXTENSION_PDF);
        extensions.put("ppt", FILE_EXTENSION_PPT);
        extensions.put("pptx", FILE_EXTENSION_PPTX);
        extensions.put("psd", FILE_EXTENSION_PSD);
        extensions.put("rtf", FILE_EXTENSION_RTF);
        extensions.put("txt", FILE_EXTENSION_TXT);
        extensions.put("xls", FILE_EXTENSION_XLS);
        extensions.put("xlsx", FILE_EXTENSION_XLSX);
        extensions.put("zip", FILE_EXTENSION_ZIP);

        EXTENSIONS_MAP = Collections.unmodifiableMap(extensions);
    }

    private OfflineItem mItem;
    private final BackendProvider mBackendProvider;
    private final ComponentName mComponentName;
    private File mFile;
    private Long mStableId;
    private boolean mIsDeletionPending;
    private Integer mFileExtensionType;

    public DownloadHistoryItemWrapper(
            OfflineItem item, BackendProvider provider, ComponentName component) {
        mItem = item;
        mBackendProvider = provider;
        mComponentName = component;
    }

    @Override
    public long getStableId() {
        if (mStableId == null) {
            // Generate a stable ID that combines the timestamp and the download ID.
            mStableId = (long) mItem.id.hashCode();
            mStableId = (mStableId << 32) + (getTimestamp() & 0x0FFFFFFFF);
        }
        return mStableId;
    }

    /** @return Whether the file will soon be deleted. */
    final boolean isDeletionPending() {
        return mIsDeletionPending;
    }

    /** Track whether or not the file will soon be deleted. */
    final void setIsDeletionPending(boolean state) {
        mIsDeletionPending = state;
    }

    /** @return Whether this download should be shown to the user. */
    boolean isVisibleToUser(int filter) {
        if (isDeletionPending()) return false;
        return filter == getFilterType() || filter == DownloadFilter.FILTER_ALL;
    }

    /** @return Item that is being wrapped. */
    Object getItem() {
        return mItem;
    }

    /**
     * Replaces the item being wrapped with a new one.
     * @return Whether or not the user needs to be informed of changes to the data.
     */
    boolean replaceItem(Object item) {
        OfflineItem offlineItem = (OfflineItem) item;
        assert mItem.id.equals(offlineItem.id);

        boolean visuallyChanged = isNewItemVisiblyDifferent(offlineItem);
        mItem = offlineItem;
        mFile = null;
        return visuallyChanged;
    }

    /** @return whether the given OfflineItem is visibly different from the current one. */
    private boolean isNewItemVisiblyDifferent(OfflineItem newItem) {
        if (mItem.progress.equals(newItem.progress)) return true;
        if (mItem.receivedBytes != newItem.receivedBytes) return true;
        if (mItem.state != newItem.state) return true;
        if (!TextUtils.equals(mItem.filePath, newItem.filePath)) return true;

        return false;
    }

    /** @return ID representing the offline item. */
    String getId() {
        return mItem.id.id;
    }

    @Override
    public long getTimestamp() {
        return mItem.creationTimeMs;
    }

    /** @return String showing where the download resides. */
    String getFilePath() {
        return mItem.filePath;
    }

    /** @return The file where the download resides. */
    public final File getFile() {
        if (mFile == null) mFile = new File(getFilePath());
        return mFile;
    }

    /** @return String to display for the hostname. */
    public final String getDisplayHostname() {
        return UrlFormatter.formatUrlForSecurityDisplay(getUrl(), false);
    }

    public String getDisplayFileName() {
        String title = mItem.title;
        if (TextUtils.isEmpty(title)) {
            return getDisplayHostname();
        } else {
            return title;
        }
    }

    /** @return Size of the file. */
    long getFileSize() {
        if (isOfflinePage()) {
            return mItem.totalSizeBytes;
        } else {
            return isComplete() ? mItem.receivedBytes : 0;
        }
    }

    /** @return URL The URL of the offline item. */
    public String getUrl() {
        return mItem.pageUrl;
    }

    /** @return {@link DownloadFilter} that represents the file type. */
    public int getFilterType() {
        return isOfflinePage() ? DownloadFilter.FILTER_PAGE
                               : DownloadFilter.fromMimeType(mItem.mimeType);
    }

    /** @return The mime type or null if the item doesn't have one. */
    public String getMimeType() {
        return mItem.mimeType;
    }

    /** @return The file extension type. See list at the top of the file. */
    public int getFileExtensionType() {
        if (isOfflinePage()) return FILE_EXTENSION_OTHER;
        if (mFileExtensionType == null) {
            int extensionIndex = getFilePath().lastIndexOf(".");
            if (extensionIndex == -1 || extensionIndex == getFilePath().length() - 1) {
                mFileExtensionType = FILE_EXTENSION_OTHER;
                return mFileExtensionType;
            }

            String extension = getFilePath().substring(extensionIndex + 1);
            if (!TextUtils.isEmpty(extension)
                    && EXTENSIONS_MAP.containsKey(extension.toLowerCase(Locale.getDefault()))) {
                mFileExtensionType = EXTENSIONS_MAP.get(extension.toLowerCase(Locale.getDefault()));
            } else {
                mFileExtensionType = FILE_EXTENSION_OTHER;
            }
        }

        return mFileExtensionType;
    }

    public boolean isOfflinePage() {
        return mItem.filter == OfflineItemFilter.FILTER_PAGE;
    }

    public boolean isSuggested() {
        return mItem.isSuggested;
    }

    /** @return How much of the download has completed, or null if there is no progress. */
    Progress getDownloadProgress() {
        return mItem.progress;
    }

    /** @return String indicating the status of the download. */
    String getStatusString() {
        return isOfflinePage() ? DownloadUtils.getOfflinePageStatusString(mItem)
                               : DownloadUtils.getDownloadStatusString(mItem);
    }

    /** @return Whether the file for this item has been removed through an external action. */
    boolean hasBeenExternallyRemoved() {
        return mItem.externallyRemoved;
    }

    /** @return Whether this download is associated with the off the record profile. */
    boolean isOffTheRecord() {
        return mItem.isOffTheRecord;
    }

    /** @return Whether the item has been completely downloaded. */
    boolean isComplete() {
        return mItem.state == OfflineItemState.COMPLETE;
    }

    /** @return Whether the item is currently paused. */
    boolean isPaused() {
        // TODO(shaktisahu): Use DownloadUtils.isDownloadPaused to make it correct.
        return mItem.state == OfflineItemState.PAUSED;
    }

    OfflineContentProvider getOfflineContentProvider() {
        Profile profile = mItem.isOffTheRecord
                ? Profile.getLastUsedProfile().getOffTheRecordProfile()
                : Profile.getLastUsedProfile().getOriginalProfile();
        return OfflineContentAggregatorFactory.forProfile(profile);
    }

    /** Called when the user wants to open the file. */
    void open() {
        if (isOfflinePage()) {
            getOfflineContentProvider().openItem(mItem.id);
            recordOpenSuccess();
        } else {
            Context context = ContextUtils.getApplicationContext();

            if (mItem.externallyRemoved) {
                Toast.makeText(context, context.getString(R.string.download_cant_open_file),
                             Toast.LENGTH_SHORT)
                        .show();
                return;
            }

            if (DownloadUtils.openFile(getFile(), getMimeType(), mItem.id.id, isOffTheRecord())) {
                recordOpenSuccess();
            } else {
                recordOpenFailure();
            }
        }
    }

    /** Called when the user tries to cancel downloading the file. */
    void cancel() {
        if (isOfflinePage()) {
            getOfflineContentProvider().cancelDownload(mItem.id);
        } else {
            mBackendProvider.getDownloadDelegate().broadcastDownloadAction(
                    mItem, DownloadNotificationService.ACTION_DOWNLOAD_CANCEL);
        }
    }

    /** Called when the user tries to pause downloading the file. */
    void pause() {
        if (isOfflinePage()) {
            getOfflineContentProvider().pauseDownload(mItem.id);
        } else {
            mBackendProvider.getDownloadDelegate().broadcastDownloadAction(
                    mItem, DownloadNotificationService.ACTION_DOWNLOAD_PAUSE);
        }
    }

    /** Called when the user tries to resume downloading the file. */
    void resume() {
        if (isOfflinePage()) {
            getOfflineContentProvider().resumeDownload(mItem.id, true);
        } else {
            mBackendProvider.getDownloadDelegate().broadcastDownloadAction(
                    mItem, DownloadNotificationService.ACTION_DOWNLOAD_RESUME);
        }
    }

    /**
     * Called when the user wants to remove the download from the backend.
     * May also delete the file associated with the download item.
     *
     * @return Whether the file associated with the download item was deleted.
     */
    boolean remove() {
        if (isOfflinePage()) {
            getOfflineContentProvider().removeItem(mItem.id);
            return true;
        } else {
            // Tell the DownloadManager to remove the file from history.
            mBackendProvider.getDownloadDelegate().removeDownload(getId(), isOffTheRecord());
            return false;
        }
    }

    protected void recordOpenSuccess() {
        RecordUserAction.record("Android.DownloadManager.Item.OpenSucceeded");
        RecordHistogram.recordEnumeratedHistogram("Android.DownloadManager.Item.OpenSucceeded",
                getFilterType(), DownloadFilter.FILTER_BOUNDARY);

        if (getFilterType() == DownloadFilter.FILTER_OTHER) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.OtherExtensions.OpenSucceeded", getFileExtensionType(),
                    FILE_EXTENSION_BOUNDARY);
        }
    }

    protected void recordOpenFailure() {
        RecordHistogram.recordEnumeratedHistogram("Android.DownloadManager.Item.OpenFailed",
                getFilterType(), DownloadFilter.FILTER_BOUNDARY);

        if (getFilterType() == DownloadFilter.FILTER_OTHER) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.OtherExtensions.OpenFailed", getFileExtensionType(),
                    FILE_EXTENSION_BOUNDARY);
        }
    }
}
