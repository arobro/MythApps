package com.mythtv.mythappsservices;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.widget.Toast;

import androidx.core.app.NotificationCompat;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;

public class MythAppsService extends Service {
    public static boolean isMyServiceRunning;
    int startMode;       // indicates how to behave if the service is killed
    IBinder binder;      // interface for clients that bind
    boolean allowRebind; // indicates whether onRebind should be used
    MythAppsWebSocketServer s = null;
    public static boolean IS_ACTIVITY_RUNNING = false;

    @Override
    public void onCreate() {
        super.onCreate();
        IS_ACTIVITY_RUNNING = true;
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel serviceChannel = new NotificationChannel(
                    "ForegroundServiceChannel",
                    "MythApps Foreground Service",
                    NotificationManager.IMPORTANCE_DEFAULT
            );
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(serviceChannel);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        createNotificationChannel();
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this,
                0, notificationIntent, 0);
        Notification notification = new NotificationCompat.Builder(this, "ForegroundServiceChannel")
                .setContentTitle("MythApps Services")
                .setContentIntent(pendingIntent)
                .build();
        startForeground(1, notification);

        //start websocket
        Context context = getApplicationContext();
        int port = 8088;
        InetSocketAddress address = new InetSocketAddress("127.0.0.1", port);

        try {
            s = new MythAppsWebSocketServer(address,  context );
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }

        s.start();
        return startMode;
    }
    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }
    @Override
    public boolean onUnbind(Intent intent) {
        return allowRebind;
    }
    @Override
    public void onRebind(Intent intent) {
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
        IS_ACTIVITY_RUNNING = false;
        Toast.makeText(this, "MythApp Service Stopped", Toast.LENGTH_LONG).show();
    }
}