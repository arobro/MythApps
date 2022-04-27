package com.mythtv.mythappsservices;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import org.java_websocket.WebSocket;
import org.java_websocket.handshake.ClientHandshake;
import org.java_websocket.server.WebSocketServer;

import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;


public class MythAppsWebSocketServer extends WebSocketServer {
    Context context;
    public MythAppsWebSocketServer(InetSocketAddress address , Context _context) throws UnknownHostException {
        super(address);

        context = _context;
        Log.d("MythAppServices","The MythApps WebSocket server is running");
    }

    @Override
    public void onOpen(WebSocket conn, ClientHandshake handshake) {
        Log.d("MythAppServices","new android device connected: " + conn.getRemoteSocketAddress());
        conn.send("Welcome");
    }

    @Override
    public void onClose(WebSocket conn, int code, String reason, boolean remote) {
        Log.d("MythAppServices","the android device has disconnected: " + conn.getRemoteSocketAddress());
    }

    @Override
    public void onMessage(WebSocket conn, String message) {
        if(message.compareTo("Kodi")==0) {
            Log.d("MythAppServices","opening Kodi");

            Intent intent = new Intent(context,MainActivity.class);
            intent.setAction(Long.toString(System.currentTimeMillis()));
            intent.setComponent(ComponentName.unflattenFromString("org.xbmc.kodi/org.xbmc.kodi.Splash"));
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent,PendingIntent.FLAG_UPDATE_CURRENT);
            context.startActivity(intent);
            ((Activity)(context)).finish();;

        }else if(message.compareTo("MythTV")==0) {
            Log.d("MythAppServices","opening mythtv");

            Intent intent = new Intent(context,MainActivity.class);
            intent.setAction(Long.toString(System.currentTimeMillis()));
            intent.setComponent(ComponentName.unflattenFromString("org.mythtv.mythfrontend/org.qtproject.qt5.android.bindings.QtActivity"));
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent,PendingIntent.FLAG_UPDATE_CURRENT);
            context.startActivity(intent);
            ((Activity)(context)).finish();;
        }else if(message.toLowerCase().contains("http")) {
            Log.d("MythAppServices","opening web browser: " + message);

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse(message));
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            ((Activity)(context)).finish();;
        }else {
            Log.d("MythAppServices","unknown: " + message);
        }
    }

    @Override
    public void onMessage(WebSocket conn, ByteBuffer message) {
    }

    @Override
    public void onError(WebSocket conn, Exception ex) {
        ex.printStackTrace();
        if (conn != null) {
        }
    }

    @Override
    public void onStart() {
        setConnectionLostTimeout(200);
    }
}