using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Text;

namespace PhoneMgr
{
    ///
    /// 文件传输过程用的结构体
    /// 
    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct SyncMsgReq
    {
        public UInt32 id;
        public UInt32 namelen;
    }

    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct SyncMsgStat
    {
        public UInt32 id;
        public UInt32 mode;
        public UInt32 size;
        public UInt32 time;
    } ;

    //[Serializable]
    //[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    //public struct SyncMsgDent
    //{
    //    public UInt32 id;
    //    public UInt32 mode;
    //    public UInt32 size;
    //    public UInt32 time;
    //    public UInt32 namelen;
    //} ;

    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct SyncMsgData
    {
        public UInt32 id;
        public UInt32 size;
    } ;

    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct SyncMsgStatus
    {
        public UInt32 id;
        public UInt32 msglen;
    } ;

    [Serializable]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct SyncSendBuf
    {
        public UInt32 id;
        public UInt32 size;
    }

    static class IDHelper
    {
        private static UInt32 MKID(char a, char b, char c, char d)
        {
            return (UInt32)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24));
        }

        public static UInt32 ID_STAT()
        {
            return MKID('S', 'T', 'A', 'T');
        }
        public static UInt32 ID_LIST()
        {
            return MKID('L', 'I', 'S', 'T');
        }
        public static UInt32 ID_ULNK() 
        {
            return MKID('U', 'L', 'N', 'K');
        }
        public static UInt32 ID_SEND() 
        {
            return MKID('S', 'E', 'N', 'D');
        }
        public static UInt32 ID_RECV()
        {
            return MKID('R', 'E', 'C', 'V');
        }
        public static UInt32 ID_DENT() 
        {
            return MKID('D', 'E', 'N', 'T');
        }
        public static UInt32 ID_DONE() 
        {
            return MKID('D', 'O', 'N', 'E');
        }
        public static UInt32 ID_DATA() 
        {
            return MKID('D', 'A', 'T', 'A');
        }
        public static UInt32 ID_OKAY()
        {
            return MKID('O', 'K', 'A', 'Y');
        }
        public static UInt32 ID_FAIL()
        {
            return MKID('F', 'A', 'I', 'L');
        }
        public static UInt32 ID_QUIT() 
        {
            return MKID('Q', 'U', 'I', 'T');
        }
    }

    
}
