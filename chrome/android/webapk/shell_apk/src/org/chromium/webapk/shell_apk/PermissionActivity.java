package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Message;
import android.os.Messenger;
import android.os.PersistableBundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;

/**
 * Created by hanxi on 10/5/17.
 */

public class PermissionActivity extends Activity implements View.OnClickListener {
    public static final String EXTRA_PERMISSIONS = "extra.permissions";
    public static final String EXTRA_GRANT_RESULTS = "extra.granted.results";
    public static final String EXTRA_RESULT_RECEIVER = "extra.result.receiver";

    private Messenger mMessenger;
    @Override
    public void onCreate(Bundle savedInstanceState,
            PersistableBundle persistentState) {
        super.onCreate(savedInstanceState, persistentState);

        Log.e("ABCD", "create permission activity!!");
        String[] permissions = savedInstanceState.getStringArray(EXTRA_PERMISSIONS);
        mMessenger = savedInstanceState.getParcelable(EXTRA_RESULT_RECEIVER);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Log.e("ABCD", "request permissions!!");
            requestPermissions(permissions, 1);
        } else {
            notify(null);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.e("ABCD", "on Request permission results!!");
        notify(grantResults);
    }

    private void notify(int[] grantResults) {
        Log.e("ABCD", "notify!!");
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
        Log.e("ABCD", "permission activity done!!");
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            finishAndRemoveTask();
        } else {
            finish();
        }
    }

    @Override
    public void onClick(View view) {
    }
}
