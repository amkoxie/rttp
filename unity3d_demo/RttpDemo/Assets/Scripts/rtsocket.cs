using System;
using System.Runtime.InteropServices;
using RTSOCKET = System.IntPtr;

namespace rtttech
{
    class rttp
    {
        public enum SocketEvent
        {
            RTTP_EVENT_CONNECT = 1,
            RTTP_EVENT_READ = 2,
            RTTP_EVENT_WRITE = 3,
            RTTP_EVENT_ERROR = 4
        }

#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
        const string DLL_NAME = "RttpDLL.dll";
#endif


#if UNITY_EDITOR_OSX || UNITY_STANDALONE_OSX
        const string DLL_NAME = "libRttpDLL.dylib";
#endif

#if UNITY_ANDROID
        const string DLL_NAME = "libRttpDLL.so";
#endif
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void socket_event_callback(RTSOCKET socket, int evt);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void send_data_callback(RTSOCKET socket, IntPtr buffer, int len, IntPtr addr, int addr_len);

        [DllImport(DLL_NAME)]
        public static extern void rt_set_callback([MarshalAs(UnmanagedType.FunctionPtr)] socket_event_callback event_call, [MarshalAs(UnmanagedType.FunctionPtr)] send_data_callback sendproc);

        [DllImport(DLL_NAME)]
        public static extern int rt_init([MarshalAs(UnmanagedType.LPStr)]string str, int len);

        [DllImport(DLL_NAME)]
        public static extern RTSOCKET rt_socket(int mode);

        [DllImport(DLL_NAME)]
        public static extern int rt_connect(RTSOCKET s, [In]byte[] to, int tolen);

        [DllImport(DLL_NAME)]
        public static extern int rt_recv(RTSOCKET s, [Out] byte[] buffer, int len, int flag);

        [DllImport(DLL_NAME)]
        public static extern int rt_send(RTSOCKET s, [In]byte[] buffer, int len, int flag);

        [DllImport(DLL_NAME)]
        public static extern void rt_close(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern IntPtr rt_incoming_packet([In]byte[] buffer, int len, [In]byte[] from, int fromlen, IntPtr userdata);

        [DllImport(DLL_NAME)]
        public static extern void rt_tick();

        [DllImport(DLL_NAME)]
        public static extern int rt_getpeername(RTSOCKET s, [Out]byte[] name, int len);

        [DllImport(DLL_NAME)]
        public static extern int rt_get_error(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern int rt_connected(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern int rt_writable(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern int rt_readable(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern int rt_setsockopt(RTSOCKET s, int optname, [In]byte[] optval, int optlen);

        [DllImport(DLL_NAME)]
        public static extern int rt_getsockopt(RTSOCKET s, int optname, [Out]byte[] optval, int optlen);

        [DllImport(DLL_NAME)]
        public static extern IntPtr rt_get_userdata(RTSOCKET s);

        [DllImport(DLL_NAME)]
        public static extern void rt_set_userdata(RTSOCKET s, IntPtr userdata);

        [DllImport(DLL_NAME)]
        public static extern int rt_pump_packet([Out]byte[] buffer, int len, IntPtr sp);

        [DllImport(DLL_NAME)]
        public static extern int rt_state_desc(RTSOCKET s, [Out]byte[] desc, int len);

        [DllImport(DLL_NAME)]
        public static extern void rt_uninit();

    }
}
