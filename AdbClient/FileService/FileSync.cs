using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Runtime.InteropServices;
using System.Text;
using IParam;
using Util;
namespace PhoneMgr
{

    class FileSync
    {
        public static bool SyncPush(string lpath, string rpath, string phonesn)
        {
            AdbClientSock sock = AdbClient.AdbConnectService("sync:", phonesn);

            if (sock == null)
                return false;
            uint mode = 0;
            if (!SyncReadMode(sock, rpath, ref mode))
                return false;

            FileInfo fi = new FileInfo(lpath);

            if (SyncSend(sock, lpath, rpath, (uint)Util.UtilTool.DateTimeToTime_t(fi.CreationTime)))
            {
                SynQuit(sock);

                return true;
            }

            return false;
        }

        #region 文件发送用的内部工具函数
        static bool SyncReadMode(AdbClientSock sock, string path, ref uint mode)
        {
            SyncMsgReq req = new SyncMsgReq();
            req.id = IDHelper.ID_STAT();
            req.namelen = (uint) path.Length;

            //if (!sock.AdbWrite(Util.SerializationUtil.SerializeObject(req))
            if(!sock.AdbWrite(UtilTool.StructToBytes(req))
                || !sock.AdbWrite(path))
            {
                return false;
            }


            SyncMsgStat stat = new SyncMsgStat();
            byte[] _stat = UtilTool.StructToBytes(stat);
            int rdlen = 0;
            if (!sock.AdbRead(_stat, _stat.Length, out rdlen))
            {
                return false;
            }
            stat = (SyncMsgStat)UtilTool.BytesToStuct(_stat, stat.GetType());
            if (stat.id != IDHelper.ID_STAT())
            {
                return false;
            }
            mode = stat.mode;
            return true;
          
        }

        static bool SyncSend(AdbClientSock sock,
            string lpath, string rpath,
            uint mtime, uint mode = 0777)
        {
            SyncMsgReq req = new SyncMsgReq();
  
            byte[] error = new byte[256];
            string tmp = "," + mode.ToString();
            req.id = IDHelper.ID_SEND();
            req.namelen = (uint)rpath.Length + (uint)tmp.Length;

            do
            {
                if (!sock.AdbWrite(UtilTool.StructToBytes(req))
                || !sock.AdbWrite(rpath)
                || !sock.AdbWrite(tmp)
                )
                {
                    break;
                }
                
                WriteFileData(sock, lpath);

                SyncMsgData data = new SyncMsgData();
                data.id = IDHelper.ID_DONE();
                data.size = mtime;
                if (!sock.AdbWrite(UtilTool.StructToBytes(data)))
                    break;

                SyncMsgStatus status = new SyncMsgStatus();
                byte[] _status = UtilTool.StructToBytes(status);

                int rdlen = 0;
                if (!sock.AdbRead(_status, out rdlen))
                    return false;

                status = (SyncMsgStatus)UtilTool.BytesToStuct(_status, status.GetType());
                if (status.id != IDHelper.ID_OKAY())
                {
                    if (status.id == IDHelper.ID_FAIL())
                    {
                        int len = (int)status.msglen;
                        if (len > 256) len = 256;
                        rdlen = 0;
                        if (!sock.AdbRead(error, len, out rdlen))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        error = Encoding.UTF8.GetBytes("unknown reason");
                    }

                    return false;
                }

                return true;

            } while (false);
            
            sock.Close();
            return false;
        }

        static void SynQuit(AdbClientSock sock)
        {
            SyncMsgReq req = new SyncMsgReq();

            req.id = IDHelper.ID_QUIT();
            req.namelen = 0;

            sock.AdbWrite(UtilTool.StructToBytes(req));
        }

        static bool WriteFileData(AdbClientSock sock, string path)
        {
            FileStream readStream = new FileStream(path, FileMode.Open);
            int numBytesToRead = (int)readStream.Length;
            int numBytesRead = 0;
            int limit = 64 * 1024;

            //包括SyncSendBuf的两个字段
            byte[] sync_data = new byte[limit + sizeof(UInt32) * 2];
            int offset = sizeof (UInt32)*2;

            SyncSendBuf sbuf = new SyncSendBuf();
            sbuf.id = IDHelper.ID_DATA();
            while (numBytesToRead > 0)
            {
                int readLen = numBytesToRead > limit ? limit : numBytesToRead;
                // Read may return anything from 0 to numBytesToRead.
                int n = readStream.Read(sync_data, offset, readLen);

                // Break when the end of the file is reached.
                if (n == 0)
                    break;
                sbuf.size = (uint)n;

                byte[] _buf = UtilTool.StructToBytes(sbuf);
                _buf.CopyTo(sync_data, 0);
                int len = _buf.Length;
                sock.AdbWrite(sync_data, offset + n);

                numBytesRead += n;
                numBytesToRead -= n;
            }

            if (numBytesRead == readStream.Length)
            {
                readStream.Close();
                return true;
            }
            else
            {
                readStream.Close();

                return false;
            }

        }
        #endregion


    }
}
