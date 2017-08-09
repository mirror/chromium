package org.chromium.chrome.browser.vr_shell.keyboard;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.IBinder;
import android.os.RemoteException;

import java.lang.ClassLoader;

import com.google.vr.keyboard.IGvrKeyboardLoader;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/** Loads the GVR keyboard SDK dynamically using the Keyboard Service. */
@JNINamespace("vr_shell")
public class GvrKeyboardLoaderClient {
    private static final String TAG = "ChromeGvrKeyboardClient";
    private static final String KEYBOARD_PACKAGE = "com.google.android.vr.inputmethod";
    private static final String LOADER_NAME =
            "com.google.vr.keyboard.GvrKeyboardLoader";

    private static IGvrKeyboardLoader staticLoader = null;
    @SuppressLint("StaticFieldLeak")
    private static Context staticRemoteContext = null;
    private static ClassLoader staticRemoteClassLoader = null;
    @SuppressLint("StaticFieldLeak")
    private static KeyboardContextWrapper staticContextWrapper = null;

    @CalledByNative
    public static long loadKeyboardSDK() {
        Log.e(TAG, "lolk loadKeyboardSDK");
        IGvrKeyboardLoader loader = getLoader();
        if (loader == null) {
            Log.e(TAG, "lolk Couldn't find GVR keyboard SDK.");
            return 0;
        }
        try {
            Log.e(TAG, "lolk calling load on sdk");
            long handle = loader.loadGvrKeyboard(BuildConstants.API_VERSION);
            Log.e(TAG, "lolk called loader.loadGvrKeyboard");
            return handle;
        } catch (RemoteException e) {
            Log.e(TAG, "lolk Couldn't load GVR keyboard SDK.", e);
            return 0;
        }
    }

    @CalledByNative
    public static void closeKeyboardSDK(long handle) {
        Log.e(TAG, "lolk closeKeyboardSDK");
        IGvrKeyboardLoader loader = getLoader();
        if (loader != null) {
            try {
                loader.closeGvrKeyboard(handle);
            } catch (RemoteException e) {
                Log.e(TAG, "Couldn't close GVR keyboard library", e);
            }
        }
    }

    private static IGvrKeyboardLoader getLoader() {
        Log.e(TAG, "lolk getLoader()");
        if (staticLoader == null) {
            Log.e(TAG, "lolk staticLoder is null");
            ClassLoader remoteClassLoader = (ClassLoader) getRemoteClassLoader();
            if (remoteClassLoader != null) {
                Log.e(TAG, "lolk binding to remote class");
                IBinder binder = newBinder(remoteClassLoader, LOADER_NAME);
                Log.e(TAG, "lolk got binder");
                staticLoader = IGvrKeyboardLoader.Stub.asInterface(binder);
                Log.e(TAG, "lolk binding protentially successfull");
            }
        }
        return staticLoader;
    }

    private static Context getRemoteContext(Context context) {
        Log.e(TAG, "lolk getRemoteContext");
        if (staticRemoteContext == null) {
            try {
                Log.e(TAG, "lolk attempting to read class");
                // The flags Context.CONTEXT_INCLUDE_CODE and Context.CONTEXT_IGNORE_SECURITY are
                // needed to be able to load classes via the classloader of the returned context.
                staticRemoteContext = context.createPackageContext(KEYBOARD_PACKAGE,
                        Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);
                Log.e(TAG, "lolk created remote context");
            } catch (NameNotFoundException e) {
                Log.e(TAG, "lolk Couldn't find remote context", e);
            }
        }
        return staticRemoteContext;
    }

    @CalledByNative
    public static Context getContextWrapper() {
        Log.e(TAG, "lolk getContextWrapper");
        Context context = ContextUtils.getApplicationContext();
        if (staticContextWrapper == null) {
            Log.e(TAG, "lolk attempting to create static context wrapper");
            staticContextWrapper = new KeyboardContextWrapper(getRemoteContext(context), context);
        }
        return staticContextWrapper;
    }

    @CalledByNative
    public static Object getRemoteClassLoader() {
        Log.e(TAG, "lolk getRemoteClassLoader");
        Context context = ContextUtils.getApplicationContext();
        if (staticRemoteClassLoader == null) {
            Log.e(TAG, "lolk class loader is null, going to set it");
            Context remoteContext = getRemoteContext(context);
            if (remoteContext != null) {
                staticRemoteClassLoader = remoteContext.getClassLoader();
            }
        }
        return staticRemoteClassLoader;
    }

    private static IBinder newBinder(ClassLoader classLoader, String className) {
        try {
            Class<?> clazz = classLoader.loadClass(className);
            return (IBinder) clazz.getConstructor().newInstance();
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "lolk class not found");
            throw new IllegalStateException("Unable to find dynamic class " + className);
        } catch (InstantiationException e) {
            Log.e(TAG, "lolk instantation exception");
            throw new IllegalStateException("Unable to instantiate the remote class " + className);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "lolk illlegal access");
            throw new IllegalStateException(
                    "Unable to call the default constructor of " + className);
        } catch (ReflectiveOperationException e) {
            Log.e(TAG, "lolk reflective operation");
            throw new IllegalStateException("Reflection error in " + className);
        }
    }

    private static class KeyboardContextWrapper extends ContextWrapper {
        private final Context keyboardContext;

        private KeyboardContextWrapper(Context keyboardContext, Context baseContext) {
            super(baseContext);
            this.keyboardContext = keyboardContext;
        }

        @Override
        public Object getSystemService(String name) {
            // As the LAYOUT_INFLATER_SERVICE uses assets from the Context, it should point to the
            // keyboard Context.
            if (Context.LAYOUT_INFLATER_SERVICE.equals(name)) {
                return keyboardContext.getSystemService(name);
            } else {
                return super.getSystemService(name);
            }
        }

        @Override
        public Resources getResources() {
            return keyboardContext.getResources();
        }

        @Override
        public AssetManager getAssets() {
            return keyboardContext.getAssets();
        }

        @Override
        public Context getApplicationContext() {
            return this;
        }

        @Override
        public ClassLoader getClassLoader() {
            return keyboardContext.getClassLoader();
        }
    }
}
