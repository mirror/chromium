package org.chromium.webapk.lib.runtime_library;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Message;
import android.os.Messenger;
import android.os.PersistableBundle;
import android.os.RemoteException;

/**
 * Created by hanxi on 10/5/17.
 */

public class PermissionActivity extends Activity {
    public static final String EXTRA_PERMISSIONS = "extra.permissions";
    public static final String EXTRA_GRANT_RESULTS = "extra.granted.results";
    public static final String EXTRA_RESULT_RECEIVER = "extra.result.receiver";

    private Messenger mMessenger;
    @Override
    public void onCreate(Bundle savedInstanceState,
            PersistableBundle persistentState) {
        super.onCreate(savedInstanceState, persistentState);

        String[] permissions = savedInstanceState.getStringArray(EXTRA_PERMISSIONS);
        mMessenger = savedInstanceState.getParcelable(EXTRA_RESULT_RECEIVER);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 1);
        } else {
            done();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Message message = new Message();
        Bundle data = new Bundle();
        data.putIntArray(EXTRA_GRANT_RESULTS, grantResults);
        message.setData(data);
        try {
            mMessenger.send(message);
        } catch (RemoteException e) {
            e.printStackTrace();
        } finally {
            done();
        }
    }

    private void done() {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            finishAndRemoveTask();
        } else {
            finish();
        }
    }
}
