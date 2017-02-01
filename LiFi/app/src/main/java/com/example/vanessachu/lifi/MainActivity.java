package com.example.vanessachu.lifi;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import java.util.Date;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class MainActivity extends AppCompatActivity {
    //private ScheduledExecutorService scheduleTaskExecutor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ScheduledExecutorService scheduleTaskExecutor= Executors.newScheduledThreadPool(1);
        Receiver Receiver1 = new Receiver();
        RecordInput Record1 = new RecordInput(Receiver1);

        int index;
        int[] array = new int[38];


        System.out.println("Hello!");
        //Record1.run();
        Thread t1 = new Thread(Record1);
        t1.start();
        System.out.println("Recorder initialized");

        //Thread t1 = new Thread(Record1).start();

        // This schedule a task to run every 10 minutes:
        /*
        scheduleTaskExecutor.scheduleAtFixedRate(Receiver1, 0, 125, TimeUnit.MICROSECONDS);


        while(true){
            for(index = 0; index < 38; index++){
                array[index] = Receiver1.getCurrentFrame(index);
            }

            for(index = 0; index < array.length; index++) {
                System.out.print(array[index]);
            }

            try {
                TimeUnit.SECONDS.sleep(1);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
           */
    } // end of onCreate()
}
