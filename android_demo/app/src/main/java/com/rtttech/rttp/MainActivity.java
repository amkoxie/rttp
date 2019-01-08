package com.rtttech.rttp;

import android.os.Bundle;
import android.os.CountDownTimer;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private Button btnStart;
    private Button btnStop;
    private EditText editServer, editRttpPort, editTcpPort;
    private EditText editRttpLatency, editTcpLatency;
    private CountDownTimer timer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        btnStart = (Button)findViewById(R.id.buttonStart);
        btnStop = (Button)findViewById(R.id.buttonStop);

        editServer  = (EditText)findViewById(R.id.editTextServer);
        editRttpPort = (EditText)findViewById(R.id.editTextRTTPPort);
        editTcpPort = (EditText)findViewById(R.id.editTextTCPPort);
        editRttpLatency = (EditText)findViewById(R.id.editTextRttpLatency);
        editTcpLatency = (EditText)findViewById(R.id.editTextTcpLatency);

        btnStart.setOnClickListener(MainActivity.this);
        btnStart.setEnabled(true);
        btnStop.setOnClickListener(MainActivity.this);
        btnStop.setEnabled(false);
        // Example of a call to a native method
        //TextView tv = (TextView) findViewById(R.id.sample_text);
        //tv.setText(stringFromJNI());

        editRttpLatency.setEnabled(false);
        editTcpLatency.setEnabled(false);

        timer = new CountDownTimer(1000, 1000) {

            public void onTick(long millisUntilFinished) {
                editRttpLatency.setText(Integer.toString(getRttpLatency()/1000));
                editTcpLatency.setText(Integer.toString(getTcpLatency()/1000));
            }

            public void onFinish() {
                timer.start();
            }
        };
    }

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

    public void onClick(View v) {
        if (v == btnStart) {
            String rttpPort = editRttpPort.getText().toString();
            String tcpPort = editTcpPort.getText().toString();
            startPing(editServer.getText().toString(), Integer.parseInt(rttpPort), Integer.parseInt(tcpPort));
            btnStop.setEnabled(true);
            btnStart.setEnabled(false);
            timer.start();
        }
        else if(v == btnStop) {
            stopPing();
            btnStart.setEnabled(true);
            btnStop.setEnabled(false);
            timer.cancel();
        }
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native void startPing(String server, int rttp_port, int tcp_port);
    public native int getRttpLatency();
    public native int getTcpLatency();
    public native void stopPing();
}
