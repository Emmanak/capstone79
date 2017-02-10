package com.example.vanessachu.lifi;

import android.Manifest;
import android.content.pm.PackageManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import java.util.Arrays;
import java.util.Date;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private ScheduledExecutorService scheduleTaskExecutor = null;
    private ScheduledFuture<?> scheduleReceiver = null;
    private Receiver Receiver1 = null;
    private RecordInput Record1 = null;

    private boolean isRecording = false;
    private static final int REQUEST_RECORD_AUDIO = 13;
    private Thread recordingThread = null;
    Handler mHandler;

    TextView outputTxt;
    private final int array_length = 12;
    private int[] mArray = new int[array_length];
    private int mIndex = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main3);
        // outputTxt = (TextView) findViewById(R.id.ArrayValue); <- THIS IS TEXT TO CHANGE
        // stopBtn = (Button) findViewByID(R.id.stopbtn); <- THIS IS STOP BUTTON
        // startBtn = (Button) findViewByID(R.id.startbtn); <- THIS IS START BUTTON

        setButtonHandlers();
        enableButtons(false);

        outputTxt = (TextView) findViewById(R.id.outputTxt);

        mHandler = new Handler(){
            @Override
            public void handleMessage(Message msg) {
                outputTxt.setText(Integer.toString(msg.arg1));
                if(mIndex == array_length - 1){
                    mArray[mIndex] = msg.arg1;
                    outputTxt.setText(Arrays.toString(mArray));
                    mIndex = 0;
                }else{
                    mArray[mIndex] = msg.arg1;
                    mIndex++;
                }
            }
        };

    } // end of onCreate()


    private void setButtonHandlers(){
        ((Button) findViewById(R.id.btnStart)).setOnClickListener(btnClick);
        ((Button) findViewById(R.id.btnStop)).setOnClickListener(btnClick);
    }


    private void enableButton(int id, boolean isEnabled) {
        ((Button) findViewById(id)).setEnabled(isEnabled);
    }


    private void enableButtons(boolean isRecording){
        enableButton(R.id.btnStart, !isRecording);
        enableButton(R.id.btnStop, isRecording);
    }

    /*
    // Start recording the audio files
     */
    private void startRecording(){
        scheduleTaskExecutor= Executors.newScheduledThreadPool(1);
        Receiver1 = new Receiver();
        Record1 = new RecordInput(Receiver1);

        AudioRecordPermission();

        if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED) {
            Thread t1 = new Thread(Record1);
            t1.start();
        }

        System.out.println("Recorder initialized");

        scheduleReceiver = scheduleTaskExecutor.scheduleAtFixedRate(Receiver1, 0, 125, TimeUnit.MICROSECONDS);

        isRecording = true;

        recordingThread = new Thread(new Runnable(){
            public void run(){
                writeAudioDataToFile();
            }
        }, "AudioRecorder Thread");

        recordingThread.start();
    }

    private void writeAudioDataToFile(){
        int index;
        int array_length = 12;
        int[] array = new int[array_length];
        int result = 0;
        Message message;

        try{
            while(isRecording){
                for(index = 0; index < array_length; index++){
                    array[index] = Receiver1.getCurrentFrame(index);
                    System.out.print(array[index]);

                    message = Message.obtain();
                    message.arg1 = array[index];
                    mHandler.sendMessage(message);
                }

                for(index = 0; index < array_length; index++) {
                    //System.out.print(array[index]);
                    //result = result * 10 + array[index];
                }

                System.out.println(" ");

                TimeUnit.MICROSECONDS.sleep(150000);
            }

        }catch (InterruptedException e) {
            e.printStackTrace();
        }
        finally {
            if(scheduleTaskExecutor != null){
                scheduleReceiver.cancel(true);
                scheduleTaskExecutor.shutdown();
            }
            if(Receiver1 != null){
                Receiver1 = null;
            }
            if(Record1 != null){
                Record1.onDestroy();
                Record1 = null;
            }
            if(recordingThread != null){
                recordingThread = null;
            }
            if(isRecording){
                isRecording = false;
            }
            if(mIndex != 0){
                mIndex = 0;
            }
        }
    }

    private void stopRecording() {
        // stops the recording activity
        if(scheduleTaskExecutor != null){
            scheduleReceiver.cancel(true);
            scheduleTaskExecutor.shutdown();
        }
        if(Receiver1 != null){
            Receiver1 = null;
        }
        if(Record1 != null){
            Record1.onDestroy();
            Record1 = null;
        }
        if(recordingThread != null){
            recordingThread = null;
        }
        if(isRecording != false){
            isRecording = false;
        }
        if(mIndex != 0){
            mIndex = 0;
        }
    }


    private View.OnClickListener btnClick = new View.OnClickListener() {
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.btnStart: {
                    enableButtons(true);
                    startRecording();
                    break;
                }
                case R.id.btnStop: {
                    MainActivity.this.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            outputTxt.setText("VALUE");
                        }
                    });

                    enableButtons(false);
                    stopRecording();
                    break;
                }
            }
        }
    };



/*
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }


    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();


        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }


        return super.onOptionsItemSelected(item);
    }
*/



    private void AudioRecordPermission(){
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_CONTACTS)
                != PackageManager.PERMISSION_GRANTED) {

            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.RECORD_AUDIO)) {

                // Show an explanation to the user *asynchronously* -- don't block
                // this thread waiting for the user's response! After the user
                // sees the explanation, try again to request the permission.

            } else {

                // No explanation needed, we can request the permission.

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.RECORD_AUDIO},
                        REQUEST_RECORD_AUDIO);

                // MY_PERMISSIONS_REQUEST_READ_CONTACTS is an
                // app-defined int constant. The callback method gets the
                // result of the request.
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_RECORD_AUDIO: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                    // permission was granted, yay! Do the
                    // contacts-related task you need to do.


                } else {

                    // permission denied, boo! Disable the
                    // functionality that depends on this permission.
                }
                return;
            }

            // other 'case' lines to check for other
            // permissions this app might request
        }
    }
}
