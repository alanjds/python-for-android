package org.p4a.minimal;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.util.Log;


// Usage: ./adb shell am broadcast -a org.p4a.minimal.START_PYTHON_SERVICE

public class PythonServiceStarter extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.v("python servicestarter", "Received intent!");
        Intent serviceIntent = new Intent(context, PythonService.class);
        context.startService(serviceIntent);
        Log.v("python servicestarter", "Service started?");
    }
}
