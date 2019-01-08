using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

public class ServerComm : MonoBehaviour {

    public enum ConnState { IDLE, CONNECTING, CONNECTED, ERROR}

    public NetworkComm network_comm;
    public static ServerComm s_instance;

    public delegate void AttackCallbackHandler(int attack_id, long latency);
    public event AttackCallbackHandler attack_callback;

    private Dictionary<int, long> attack_dict = new Dictionary<int, long>();
    private int attack_id = 0;
    private ConnState conn_state = ConnState.IDLE;
    private string conn_status = "";
    // Use this for initialization
    void Start () {
        
	}
	
	// Update is called once per frame
	void Update () {
		
	}

    void Awake()
    {
        s_instance = this;

        NetworkComm.s_connected_callback += OnServerConnectedCallback;
        NetworkComm.s_error_callback += OnErrorCallbackHandler;
        NetworkComm.s_data_callback += OnServerDataCallback;

        network_comm.Initialize();
    }

    public void ConnectToServer()
    {
        if (conn_state == ConnState.CONNECTING || conn_state == ConnState.CONNECTED)
            return;

        network_comm.ConnectServer();

        conn_state = ConnState.CONNECTING;

        lock (conn_status)
        {
            conn_status = "connecting";
        }
    }

    public void Attack()
    {
        if (conn_state != ConnState.CONNECTED)
        {
            Debug.Log("not connected");
            return;
        }

        ++attack_id;
        attack_dict[attack_id] = Utility.GetMilliSecondSinceEpoch();

        System.Random rnd = new System.Random();
        int bytes = rnd.Next(100, 1000);
        byte[] data = new byte[bytes];
        Array.Copy(BitConverter.GetBytes(bytes), 0, data, 0, 4);
        Array.Copy(BitConverter.GetBytes(attack_id), 0, data, 4, 4);

        network_comm.SendData(data);
    }

    public ConnState GetState()
    {
        return conn_state;
    }

    public string GetStateDesc()
    {
        lock (conn_status)
        {
            return conn_status;
        }
    }

    public void OnServerConnectedCallback()
    {
        Debug.Log("server connected");
        conn_state = ConnState.CONNECTED;

        lock (conn_status)
        {
            conn_status = "connected";
        }
    }

    public void OnErrorCallbackHandler(int err)
    {
        lock (conn_status)
        {
            conn_status = string.Format("error: {0}", err);
            Debug.Log(conn_status);
        }
        
        conn_state = ConnState.ERROR;
        
    }

    public void OnServerDataCallback(byte[] data, int len)
    {
        int attack_id = BitConverter.ToInt32(data, 4); 

        if (attack_dict.ContainsKey(attack_id))
        {
            long latency = Utility.GetMilliSecondSinceEpoch() - attack_dict[attack_id];
            attack_dict.Remove(attack_id);

            Debug.Log(String.Format("attack id {0}, latency:{1}", attack_id, latency));

            attack_callback.Invoke(attack_id, latency);
        }
        else
        {
            Debug.Log(String.Format("attack id {0} not found", attack_id));
        }
    }
}
