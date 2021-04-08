
package com.SVFUnityPlugin;

import android.app.Activity;
import android.widget.TextView;
import android.os.*;
import android.os.Bundle;
import android.os.storage.*;
import android.content.Context.*;
import android.net.Uri;
import android.util.Log;
import android.graphics.*;
import android.view.*;
import android.content.res.*;
import android.media.*;
import java.util.concurrent.ConcurrentHashMap;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;

import com.google.android.vending.expansion.zipfile.*;

public class JavaPlugin implements SurfaceTexture.OnFrameAvailableListener {

    private static native void onCreated(JavaPlugin javaplugin, AssetManager assetManager, boolean usingObb);
    private static native void onCreatedUE4(JavaPlugin javaplugin, AssetManager assetManager);
    private static native void onFrameAvailableNative(SurfaceTexture surfaceTexture);
    private static native void onDownloadComplete(String downloadLocation, long instanceID);
    private static native void SetupHCapStreamMediaExtractor(byte[] fileData, AssetFileDescriptor afd, long instanceId);
    public static final String TAG = "SVFUnityPlugin";
    
    static {
        System.loadLibrary("SVFUnityPlugin");
    }

    //private StorageManager storageManager;
    private Activity activity;
    ZipResourceFile expansionFile;
        
    public JavaPlugin(Activity unityActivity, String obbPath) 
    {
        Log.d(TAG, "--------- JavaPlugin created ---------");

        activity = unityActivity;

        if (obbPath != null) 
        {
        	try 
        	{
                Log.d(TAG, "JavaPlugin: opening ZipResourceFile for obb path: " + obbPath);
                // Get a ZipResourceFile representing a specific expansion file
                expansionFile = new ZipResourceFile(obbPath);
            }

            catch (IOException e) 
        	{
                Log.d(TAG, "JavaPlugin: failed to open ZipResourceFile: " + e.getMessage());
            }
        }

        onCreated(this, unityActivity.getResources().getAssets(), expansionFile != null);
    }

    public JavaPlugin(Activity unityActivity, boolean ue4) {
        Log.d(TAG, "--------- JavaPlugin created for UE4 ---------");
        activity = unityActivity;
        onCreatedUE4(this, unityActivity.getResources().getAssets());
    }

    public void onBackPressed() {
        // instead of calling UnityPlayerActivity.onBackPressed() we just ignore the back button event
        // super.onBackPressed();
    }

    synchronized public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        /* For simplicity, SurfaceTexture calls here when it has new
        * data available.  Call may come in from some random thread,
        * so let's be safe and use synchronize. No OpenGL calls can be done here.
        */
        //Log.d("SVFUnityPlugin", "SVFUnityPluginActivity.onFrameAvailable called");


        //Log.d("SVFUnityPlugin", "calling surface.updateTexImage");        
        onFrameAvailableNative(surfaceTexture);        
    }

    public AudioTrack createAudioTrack(int sampleRate) {

        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);

        AudioTrack audioTrack = new  AudioTrack(AudioManager.STREAM_MUSIC,
                sampleRate,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM);                

        return audioTrack;
    }

    public SurfaceTexture createSurfaceTexture(int glTexId) {
        //Log.d("SVFUnityPlugin", "SVFUnityPluginActivity.createSurfaceTexture called with glTexId = " + glTexId);
        SurfaceTexture surfaceTexture = new SurfaceTexture(glTexId);
        surfaceTexture.setOnFrameAvailableListener(this);
        return surfaceTexture;
    }

	public void releaseSurfaceTexture(SurfaceTexture surfaceTexture) {
		//Log.d("SVFUnityPlugin", "releaseSurfaceTexture");
        surfaceTexture.setOnFrameAvailableListener(null);
        surfaceTexture.release();
    }

    public Surface createSurfaceTextureSurface(SurfaceTexture surfaceTexture) {        
        Surface surfaceTextureSurface = new Surface(surfaceTexture);
        return surfaceTextureSurface;
    }

    public void releaseSurfaceTextureSurface(Surface surface) {
        surface.release();
    }

    private static int tempCounter = 0;
    private static final String tempDir = System.getProperty("java.io.tmpdir");
    private ConcurrentHashMap<Long, DownloadTask> downloadTasks = new ConcurrentHashMap<Long, DownloadTask>();

    class DownloadTask implements Runnable 
    {
        URL url;
        File file;
        Long instance;
        boolean cancel = false;
        JavaPlugin javaPlugin;
        Thread thread;
        String segmentName;

    	public DownloadTask(
                String directory,
    			String urlString,
    			String segmentName,
    			long instance,
    			JavaPlugin javaPlugin) 
    	{
            try
            {
                this.javaPlugin = javaPlugin;
                this.segmentName = segmentName;

                this.instance = instance;
                url = new URL(urlString);

                String destFilePath = directory + "/" + segmentName;
                file = new File(destFilePath);
                file.mkdirs();
                if (file.exists())
                {
                    file.delete();
                }
                file.deleteOnExit();
            }
            
            catch (Exception e)
            {
                Log.e(TAG, "downloadNextBuffer error");
                e.printStackTrace();
                onDownloadComplete(null, instance);
            }
    	}

        private void onCancel()
        {
            Log.i(TAG, "download cancelled: " + url.toString());
            javaPlugin.downloadTasks.remove(instance);
            onDownloadComplete(null, instance);
        }

    	public void run() 
    	{
            try
            {
                HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
                InputStream inputStream = urlConnection.getInputStream();

                OutputStream output = new FileOutputStream(file);

                int n = 0;
                byte[] buffer = new byte[256 * 1024];
                while ((n = inputStream.read(buffer)) != -1)
                {
                    if (cancel)
                    {
                        output.close();
                        onCancel();
                        return;
                    }

                    output.write(buffer, 0, n);
                }
                output.close();

                Log.i(TAG, segmentName + " downloaded to: " + file.toString());
                
                if (cancel)
                {
                    onCancel();
                }

                else
                {
                    onDownloadComplete(file.toString(), instance);
                }
            }
            catch (Exception e)
            {
                Log.e(TAG, "downloadNextBuffer error");
                e.printStackTrace();
                javaPlugin.downloadTasks.remove(instance);
                onDownloadComplete(null, instance);
            }
    	}
    }

    public void cancelDownload(long instance)
    {        
        if (downloadTasks.containsKey(instance))
        {
            DownloadTask task = downloadTasks.remove(instance);
            task.cancel = true;

            Log.i(TAG, "cancelDownload: " + instance);

            try
            {
                task.thread.join();
            }

            catch (Exception e)
            {
                Log.e(TAG, "task.wait error");
                e.printStackTrace();
            }
        }

        else
        {
            Log.i(TAG, "no download found for instance: " + instance);
        }
    }

    public void downloadFileFromUrl(String captureName, String urlString, String segmentName, long instance)
    {
        //Log.i(TAG, "downloadFileFromUrl: " + urlString);
        String directory = tempDir + "/" + instance + "/" + captureName;

        DownloadTask downloadTask = new DownloadTask(directory, urlString, segmentName, instance, this);
        downloadTasks.put(instance, downloadTask);
        downloadTask.thread = new Thread(downloadTask);
        downloadTask.thread.start();
    }

    public String downloadFileFromUrlBlocking(final String urlString)
    {
        try
        {
            Log.i(TAG, "downloadFileFromUrlBlocking: " + urlString);

            URL url = new URL(urlString);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            InputStream inputStream = urlConnection.getInputStream();

            File file = File.createTempFile("tempfile" + tempCounter++, ".tmp");
            file.deleteOnExit();
            OutputStream output = new FileOutputStream(file);

            int n = 0;
            byte[] buffer = new byte[4096];
            while ((n = inputStream.read(buffer)) != -1)
            {
                output.write(buffer, 0, n);
            }
            output.close();

            Log.i(TAG, urlString + " downloaded to: " + file.toString());
            return file.toString();
        }

        catch (Exception e)
        {
            Log.e(TAG, "downloadNextBuffer error");
            e.printStackTrace();
            return null;
        }
    }

    static final int XTRA_ATOM_SIZE = 1024;

    public void getAssetFileDescriptorFromObb(String assetName, long instanceId)
    {  
        if (expansionFile != null) {
            try {
                AssetFileDescriptor afd = expansionFile.getAssetFileDescriptor("assets/" + assetName);
                FileInputStream ifs = afd.createInputStream();
                // read m_xtraAtom
                byte[] m = new byte[XTRA_ATOM_SIZE];
                ifs.skip(afd.getLength() - XTRA_ATOM_SIZE);
                ifs.read(m, 0, XTRA_ATOM_SIZE);
                // Send the m_xtraAtom buffer and the asset file descriptor to the
                // native plugin
                SetupHCapStreamMediaExtractor(m, afd, instanceId);
                // close the asset file descriptor
                afd.getParcelFileDescriptor().close();
            } catch (Exception e) {
                Log.i(TAG, "JavaPlugin.getAssetFileDescriptorObb failed with assetName = " + assetName);
            }
        }

    }

    public void getAssetFileDescriptorAndroid10(String assetPath, long instanceId)
    {
        Log.i(TAG, "getAssetFileDescriptorAndroid10");
        try {
            Uri uri = Uri.fromFile(new File(assetPath));
            AssetFileDescriptor afd = activity.getContentResolver().openAssetFileDescriptor(uri, "rw");
            FileInputStream ifs = afd.createInputStream();
            // read m_xtraAtom
            byte[] m = new byte[XTRA_ATOM_SIZE];
            ifs.skip(afd.getLength() - XTRA_ATOM_SIZE);
            ifs.read(m, 0, XTRA_ATOM_SIZE);
            // Send the m_xtraAtom buffer and the asset file descriptor to the
            // native plugin
            SetupHCapStreamMediaExtractor(m, afd, instanceId);
            // close the asset file descriptor
            afd.getParcelFileDescriptor().close();
        } catch (Exception e) {
            Log.i(TAG, "JavaPlugin.getAssetFileDescriptorAndroid10 failed with assetPath = " + assetPath);
        }
    }

    public void closeAssetFileDescriptor(AssetFileDescriptor afd) {
        try
        {
            afd.getParcelFileDescriptor().close();
        }
        catch (IOException ex) {
            Log.i(TAG, "JavaPlugin.closeAssetFileDescriptor failed");
        }
    }
}
