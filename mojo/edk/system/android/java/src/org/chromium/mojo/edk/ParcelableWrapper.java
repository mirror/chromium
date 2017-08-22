// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.mojo.edk;
import android.os.Parcel;
import android.os.Parcelable;
/**
 * A Parcelable and its associated ID.
 * Used on the ParcelableChannel to send Parcelable's over a Mojo channel
 */
public class ParcelableWrapper implements Parcelable {
    private final int mId;
    private final Parcelable mParcelable;
    public ParcelableWrapper(int id, Parcelable parcelable) {
        mId = id;
        mParcelable = parcelable;
    }
    ParcelableWrapper(Parcel in) {
        mId = in.readInt();
        mParcelable = in.readParcelable(getClass().getClassLoader());
    }
    public int getId() {
        return mId;
    }
    public Parcelable getParcelable() {
        return mParcelable;
    }
    @Override
    public int describeContents() {
        return 0;
    }
    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mId);
        dest.writeParcelable(mParcelable, 0);
    }
    public static final Parcelable.Creator<ParcelableWrapper> CREATOR =
            new Parcelable.Creator<ParcelableWrapper>() {
                @Override
                public ParcelableWrapper createFromParcel(Parcel in) {
                    return new ParcelableWrapper(in);
                }
                @Override
                public ParcelableWrapper[] newArray(int size) {
                    return new ParcelableWrapper[size];
                }
            };
}
