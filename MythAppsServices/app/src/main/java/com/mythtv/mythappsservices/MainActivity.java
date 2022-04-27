package com.mythtv.mythappsservices;

import androidx.appcompat.app.AppCompatActivity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    MythAppsWebSocketServer s = null;
    Context context;
    boolean restart = false;
    int app = 0;

   int ACTION_MANAGE_OVERLAY_PERMISSION_REQUEST_CODE = 24;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        context = this;

        checkPermission();

        startForegroundService(new Intent(this, MythAppsService.class));
        Log.d("MythAppServices","oncreate() ");
    }

    public void openMyth() {
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(context, MainActivity.class);
                intent.setAction(Long.toString(System.currentTimeMillis())); //intent.FLAG_CANCEL_CURRENT
                intent.setComponent(ComponentName.unflattenFromString("org.xbmc.kodi/org.xbmc.kodi.Splash"));
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
                startActivity(intent);
                finish();

                try{
                    Thread.sleep(1500);
                }catch(InterruptedException e){
                    System.out.println(e);
                }

                intent = new Intent(context, MainActivity.class);
                intent.setAction(Long.toString(System.currentTimeMillis())); //intent.FLAG_CANCEL_CURRENT
                intent.setComponent(ComponentName.unflattenFromString("org.mythtv.mythfrontend/org.qtproject.qt5.android.bindings.QtActivity"));
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                pendingIntent = PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
                startActivity(intent);
                finish();
            }
        }, 1000 );//time in miliseconds
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == ACTION_MANAGE_OVERLAY_PERMISSION_REQUEST_CODE) {
            if (!Settings.canDrawOverlays(this)) {  // No permission
                checkPermission();
            }

        }
    }

    public void checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (!Settings.canDrawOverlays(this)) {
                Toast.makeText(this, "Please find MythApps Services and enable 'appear on top.'", Toast.LENGTH_LONG).show();
                Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                        Uri.parse("package:" + getPackageName()));
                startActivityForResult(intent, ACTION_MANAGE_OVERLAY_PERMISSION_REQUEST_CODE);
            }else { //have permission
                Toast.makeText(this, "Remember to disable power optimization for Mythfrontend and Kodi.", Toast.LENGTH_LONG).show();
                Toast.makeText(this,"Settings->Apps->:->Special Access->Optimise Battery Usage->All->Mythfrontend ->Off", Toast.LENGTH_LONG).show();
                openMyth();
            }
        }
    }
}