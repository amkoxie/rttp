using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System;
using rtttech;
using System.Runtime.InteropServices;
using RTSOCKET = System.IntPtr;


public class Utility
{
    static public long GetMilliSecondSinceEpoch()
    {
        TimeSpan ts = DateTime.Now - DateTime.Parse("1970-1-1");
        return (long)ts.TotalMilliseconds;
    }
}

public class NetworkComm : MonoBehaviour
{
    public static NetworkComm s_instance = null;
    private static Thread s_thread = null;
    private static Queue<byte[]> s_udp_send_queue = new Queue<byte[]>();
    private static UdpClient s_udp_socket = new UdpClient();
    private static IPEndPoint s_server_addr = null;
    private static bool s_udp_sending = false;
    private static RTSOCKET s_rtsocket = (RTSOCKET)0;
    private static string s_status = "idle";
    private static bool s_rttp_waiting_send = false;
    private static Queue<byte[]> s_rttp_send_queue = new Queue<byte[]>();

    private static byte[] s_received_data = new byte[4096];
    private static int s_received_size = 0;
    private static int s_sent_size = 0;

    public enum Command { INIT, CONNECT, DISCONNECT, SEND_DATA, UDP_DATA, UDP_SEND, EXIT }
    public struct MessageQueueItem
    {
        public Command cmd;
        public Action action;
    }

    private static Queue<MessageQueueItem> s_thread_msg_queue = new Queue<MessageQueueItem>();
    private static ManualResetEvent s_queue_event = new ManualResetEvent(false);

    public delegate void ServerConnectedCallbackHandler();
    public static event ServerConnectedCallbackHandler s_connected_callback;

    public delegate void ErrorCallbackHandler(int err);
    public static event ErrorCallbackHandler s_error_callback;

    public delegate void ServerDataCallbackHandler(byte[] data, int len);
    public static event ServerDataCallbackHandler s_data_callback;

    public void OnSocketEvent(RTSOCKET socket, int evt)
    {
        if (evt == (int)rttp.SocketEvent.RTTP_EVENT_READ)
        {
            OnRead(socket);
        }
        else if(evt == (int)rttp.SocketEvent.RTTP_EVENT_WRITE)
        {
            OnWrite(socket);
        }
        else if(evt == (int)rttp.SocketEvent.RTTP_EVENT_CONNECT)
        {
            OnConnected(socket);
        }
        else if (evt == (int)rttp.SocketEvent.RTTP_EVENT_ERROR)
        {
            int err = rttp.rt_get_error(socket);
            OnError(socket, err);
        }
        else
        {

        }
    }

    public void OnConnected(RTSOCKET socket)
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (socket != s_rtsocket)
            return;

        s_status = "connected";

        s_connected_callback.Invoke();

        Debug.Log("connect server success");
    }

    public void OnRead(RTSOCKET socket)
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (socket != s_rtsocket)
            return;

        RecvServerResponse();
    }

    public void OnWrite(RTSOCKET socket)
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (socket != s_rtsocket)
            return;

        SendUserData();
    }


    public void OnError(RTSOCKET socket, int errcode)
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (socket != s_rtsocket)
            return;

        s_status = string.Format("error:{0}", errcode);
        s_error_callback.Invoke(errcode);

        Debug.Log(s_status);
    }

    public void SendUserData()
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        while (s_rttp_send_queue.Count > 0 && !s_rttp_waiting_send)
        {
            byte[] data = s_rttp_send_queue.Peek();

            int cur_send_size = data.Length - s_sent_size;
            byte[] cur_send_data = new byte[cur_send_size];
            Array.Copy(data, s_sent_size, cur_send_data, 0, cur_send_size);
            int ret = rttp.rt_send(s_rtsocket, cur_send_data, cur_send_size, 0);
            Debug.Log(String.Format("data len: {0}, already sent: {1}， cur send: {2}, send return:{3}",
                data.Length, s_sent_size, cur_send_size, ret));
            if (ret > 0)
            {
                s_rttp_waiting_send = false;
                s_sent_size += ret;
                if (s_sent_size == data.Length)
                {
                    s_rttp_send_queue.Dequeue();
                    s_sent_size = 0;
                }
            }
            else if (ret == -1)
            {
                s_rttp_waiting_send = true;
                return;
            }
            else
            {
                //error happened
                s_rttp_waiting_send = false;
                s_error_callback.Invoke(ret);
                s_sent_size = 0;
                return;
            }
        }
    }

    public void SendDataImp(RTSOCKET socket, IntPtr buffer, int len, IntPtr addr, int addr_len)
    {
        //Debug.Log(String.Format("send data callback, data bytes: {0}", len));
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        byte[] data = new byte[len];
        Marshal.Copy(buffer, data, 0, len);
        if (s_udp_sending)
        {
            s_udp_send_queue.Enqueue(data);
        }
        else
        {
            s_udp_socket.BeginSend(data, len, s_server_addr, new System.AsyncCallback(OnUdpSend), null);
            s_udp_sending = true;
        }
    }

    public void RecvServerResponse()
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        while (true)
        {
            int ret = 0;
            byte[] buff;
            if (s_received_size < 4)
            {
                int recv_size = 4 - s_received_size;
                buff = new byte[recv_size];
                ret = rttp.rt_recv(s_rtsocket, buff, recv_size, 0);
            }
            else
            {
                int packet_size = BitConverter.ToInt32(s_received_data, 0);
                int recv_size = packet_size - s_received_size;
                buff = new byte[recv_size];
                ret = rttp.rt_recv(s_rtsocket, buff, recv_size, 0);
            }

            if (ret > 0)
            {
                Array.Copy(buff, 0, s_received_data, s_received_size, ret);
                s_received_size += ret;

                if (s_received_size > 4)
                {
                    if (s_received_size == BitConverter.ToInt32(s_received_data, 0))
                    {
                        Debug.Log("recevied attack response");
                        s_data_callback.Invoke(s_received_data, s_received_size);
                        s_received_size = 0;
                    }
                }
            }
            else if (ret == 0)
            {
                //receive return 0 means connection has been closed by remote server.
                s_status = "closed";
                s_error_callback.Invoke(0);
                return;
            }
            else if (ret == -1)
            {
                //waiting for next OnRead call back.
                return;
            }
            else
            {
                s_status = string.Format("error:{0}", ret);
                s_error_callback.Invoke(ret);
                return;
            }
        }
    }

    // Use this for initialization
    void Start()
    {

    }
    // Update is called once per frame
    void Update()
    {
        
    }

    void Awake()
    {
        s_instance = this;

        s_thread = new Thread(ThreadProc);
        s_thread.Start();
    }

    void OnDestroy()
    {
        PostThreadMessage(Command.EXIT, new Action(() => {}));
        s_thread.Join();
        s_thread = null;
    }

    static void PostThreadMessage(Command cmd, Action act)
    {
        lock(s_thread_msg_queue)
        {
            MessageQueueItem item = new MessageQueueItem();
            item.cmd = cmd;
            item.action = act;

            s_thread_msg_queue.Enqueue(item);
        }

        s_queue_event.Set();
    }

    static void OnUdpReceive(System.IAsyncResult result)
    {
        IPEndPoint ep = new IPEndPoint(IPAddress.Any, 0);
        byte[] data = s_udp_socket.EndReceive(result, ref ep);

        //Debug.Log(String.Format("udp received {0} bytes", data.Length));

        Action act = new Action(() => {
            rttp.rt_incoming_packet(data, data.Length, null, 0, (System.IntPtr)0);
        }
        );
        PostThreadMessage(Command.UDP_DATA, act);

        s_udp_socket.BeginReceive(new System.AsyncCallback(OnUdpReceive), null);
    }


    static void SendPacketInUDPQueue()
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (s_udp_send_queue.Count > 0)
        {
            byte[] data = s_udp_send_queue.Dequeue();
            s_udp_socket.BeginSend(data, data.Length, s_server_addr, new System.AsyncCallback(OnUdpSend), null);
            s_udp_sending = true;
        }
    }

    static void OnUdpSend(System.IAsyncResult result)
    {
        Action act = new Action(() => {
            s_udp_sending = false;
            SendPacketInUDPQueue();
        }    
        );
        PostThreadMessage(Command.UDP_SEND, act);
    }

    public void Initialize()
    {
        Action act = new Action(() => this.DoInitialize());
        PostThreadMessage(Command.INIT, act);
    }

    public void DoInitialize()
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        s_udp_socket.Client.Blocking = false;
        s_udp_socket.BeginReceive(new System.AsyncCallback(OnUdpReceive), null);

        rttp.rt_init("", 0);

        rttp.rt_set_callback(new rttp.socket_event_callback(OnSocketEvent), new rttp.send_data_callback(SendDataImp));
        Debug.Log("set callback success");
    }

    public void ConnectServer(string strServer, int port)
    {
        Action act = new Action(() => this.DoConnectServer(strServer, port));
        PostThreadMessage(Command.CONNECT, act);
    }

    public void DoConnectServer(string strServer, int port)
    {
        Debug.Assert(Thread.CurrentThread.ManagedThreadId == s_thread.ManagedThreadId);

        if (s_rtsocket != (RTSOCKET)0)
        {
            rttp.rt_close(s_rtsocket);
        }

        s_server_addr = new IPEndPoint(IPAddress.Parse(strServer), port);

        s_rtsocket = rttp.rt_socket(0);
        if (s_rtsocket != (RTSOCKET)0)
        {
            Debug.Log("create rttp socket success");
        }

        Debug.Log("rttp connecing server");
        rttp.rt_connect(s_rtsocket, null, 0);

    }

    public void Disconnect()
    {
        Action act = new Action(() => {
            rttp.rt_close(s_rtsocket);
            s_rtsocket = (RTSOCKET)0;

            s_rttp_send_queue.Clear();
            s_sent_size = 0;
            s_received_size = 0;

            s_udp_send_queue.Clear();
        }
        );

        PostThreadMessage(Command.DISCONNECT, act);
    }

    public void SendData(byte[] data)
    {
        Action act = new Action(() => {
            s_rttp_send_queue.Enqueue(data);
            SendUserData();
        }
        );

        PostThreadMessage(Command.SEND_DATA, act);
    }

    public string GetConnState()
    {
        return s_status;
    }

    static void ThreadProc()
    {
        Debug.Log("rttp working thread running");
        long last_input_tick = Utility.GetMilliSecondSinceEpoch();
        while (true)
        {
            long now = Utility.GetMilliSecondSinceEpoch();
            long interval =  now - last_input_tick;
            if (interval > 10 || interval < 0/*in case clock back*/)
            {
                rttp.rt_tick();
                last_input_tick = now;
            }

            MessageQueueItem item = new MessageQueueItem();
            bool found_item = false;
            lock (s_thread_msg_queue)
            {
                if (s_thread_msg_queue.Count > 0)
                {
                    item = s_thread_msg_queue.Dequeue();
                    found_item = true;
                }
            }

            if (found_item)
            {
                if (item.cmd == Command.EXIT)
                {
                    break;
                }
                else
                {
                    item.action();
                    continue;
                }
            }
            else
            {
                s_queue_event.WaitOne(10);
            }

        }

        Debug.Log("rttp working thread exit");
    }
}