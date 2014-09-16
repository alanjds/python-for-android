package org.p4a.minimal;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;

import android.content.Intent;
import android.content.Context;
import android.content.res.AssetManager;

import android.os.Bundle;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;

import android.util.Log;


public final class PythonService extends Service implements Runnable {

    static {
        System.loadLibrary("python2.7");
        System.loadLibrary("native-service");
    }

    // Thread for Python code
    private Thread pythonThread = null;
    // Python environment variables
    private String androidPrivate;
    private String androidArgument;
    private String pythonHome;
    private String pythonPath;
    // Argument to pass to Python code,
    private String pythonServiceArgument;

    public static Service mService = null;

    @Override
    public void onCreate() {
        Log.i("python service", "onCreate called");
        super.onCreate();
        Log.i("python service", "onCreate ended");
    }

    @Override
    public IBinder onBind(Intent arg) {
        Log.i("python service", "onBind called");
        Log.i("python service", "onBind ended");
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("python service", "onStartCommand called");
        if (pythonThread != null) {
            Log.v("python service", "service exists, do not start again");
            return START_NOT_STICKY;
        }

        Bundle extras = intent.getExtras();
        //androidPrivate = extras.getString("androidPrivate");
        //// service code is located in service subdir
        //androidArgument = extras.getString("androidArgument") + "/service";
        //pythonHome = extras.getString("pythonHome");
        //pythonPath = extras.getString("pythonPath");

        //pythonServiceArgument = extras.getString("pythonServiceArgument");
        //String serviceTitle = extras.getString("serviceTitle");
        //String serviceDescription = extras.getString("serviceDescription");

        pythonThread = new Thread(this);
        pythonThread.start();

        //Context context = getApplicationContext();
        //Notification notification = new Notification(context.getApplicationInfo().icon,
        //        serviceTitle,
        //        System.currentTimeMillis());
        //Intent contextIntent = new Intent(context, PythonActivity.class);
        //PendingIntent pIntent = PendingIntent.getActivity(context, 0, contextIntent,
        //        PendingIntent.FLAG_UPDATE_CURRENT);
        //notification.setLatestEventInfo(context, serviceTitle, serviceDescription, pIntent);
        //startForeground(1, notification);
        Log.i("python service", "onStartCommand ended");

        return START_REDELIVER_INTENT;
    }

    @Override
    public void onDestroy() {
        Log.i("python service", "onDestroy called");
        super.onDestroy();
        pythonThread = null;
        Process.killProcess(Process.myPid());
        Log.i("python service", "onDestroy ended");
    }

    @Override
    public void run(){
        Log.i("python service", "run() called");

        this.mService = this;

        nativeServiceStart(
            getPackageCodePath(),
            getApplicationInfo().nativeLibraryDir,
            getFilesDir().getAbsolutePath(),
            //getExternalFilesDir().getAbsolutePath(),
            getFilesDir().getAbsolutePath(),
            getAssets()
        );

        Log.i("python service", "run() ended");
    }

    // Native part
    public static native int nativeServiceStart(String packageCodePath,
                                                String nativeLibraryDir,
                                                String filesDir,
                                                String externalFilesDir,
                                                AssetManager assets);
    //public static native void nativeInitJavaEnv();
}
