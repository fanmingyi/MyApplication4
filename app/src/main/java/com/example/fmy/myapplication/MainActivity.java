package com.example.fmy.myapplication;

import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {


    private TextView tv;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        tv = (TextView) findViewById(R.id.sample_text);

    }


    public void onClick(View view) {

        File inFile = new File(Environment.getExternalStorageDirectory(),"test.wav");
Log.e("文件路径",inFile.exists()+"");
        File outFile = new File(Environment.getExternalStorageDirectory(),"testout.wav");

        try
        {
            ProcessTask task = new ProcessTask();
            ProcessTask.Parameters params = task.new Parameters();
            // 解析进程参数 parse processing parameters
            params.inFileName = inFile.getAbsolutePath();
            params.outFileName = outFile.getAbsolutePath();
            params.tempo = 10;
            params.pitch = -30;

            //在界面更新状态
//            appendToConsole("Process audio file :" + params.inFileName +" => " + params.outFileName);
//            appendToConsole("Tempo = " + params.tempo);
//            appendToConsole("Pitch adjust = " + params.pitch);

            Toast.makeText(this, "Starting to process file " + params.inFileName + "...", Toast.LENGTH_SHORT).show();

            //在后台线程中启动SoundTouch处理
            task.execute(params);
//			task.doSoundTouchProcessing(params);	//这个将运行在主线程

        }
        catch (Exception exp)
        {
            exp.printStackTrace();
        }

    }

    //将执行SoundTouch处理的助手类。 由于处理可能需要一些时间，请在后台线程中运行，以避免挂起UI。
    protected class ProcessTask extends AsyncTask<ProcessTask.Parameters, Integer, Long>
    {
        ///Helper类来存储SoundTouch文件处理参数
        public final class Parameters
        {
            String inFileName;
            String outFileName;
            float tempo;
            float pitch;
        }



        /// 执行SoundTouch处理功能
        public final long doSoundTouchProcessing(Parameters params)
        {

            SoundTouch st = new SoundTouch();
            st.setTempo(params.tempo);
            st.setPitchSemiTones(params.pitch);
            Log.i("SoundTouch", "process file " + params.inFileName);
            long startTime = System.currentTimeMillis();
            int res = st.processFile(params.inFileName, params.outFileName);
            long endTime = System.currentTimeMillis();
            float duration = (endTime - startTime) * 0.001f;

            Log.i("SoundTouch", "process file done, duration = " + duration);
//            appendToConsole("Processing done, duration " + duration + " sec.");
            if (res != 0)
            {
                String err = SoundTouch.getErrorString();
//                appendToConsole("Failure: " + err);
                Log.e("Filure",err);
                return -1L;
            }


            //播放文件如果是可取的
//            if (checkBoxPlay.isChecked())
//            {
                playWavFile(params.outFileName);
//            }
            return 0L;
        }




        //由系统调用的过载功能执行后台处理
        @Override
        protected Long doInBackground(Parameters... aparams)
        {
            return doSoundTouchProcessing(aparams[0]);
        }

    }
    /// 播放音频文件
    protected void playWavFile(String fileName)
    {
        File file2play = new File(fileName);
        Intent i = new Intent();
        i.setAction(android.content.Intent.ACTION_VIEW);
        i.setDataAndType(Uri.fromFile(file2play), "audio/wav");
        startActivity(i);
    }
}
