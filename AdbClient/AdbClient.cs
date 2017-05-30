using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using IParam;
using Microsoft.SqlServer.Server;
using Util;

namespace PhoneMgr
{
    class AdbClient
    {
       
        public static bool Forward(string sn, int locport, int rmtport)
        {
            string service = "host-serial:" + sn + ":forward:tcp:" + locport.ToString() + ";tcp:" + rmtport.ToString();

            AdbClientSock sock = AdbConnectService(service, sn);

            if (sock != null)
            {
                sock.Close();

                return true;
            }

            return false;
        }

        public static bool UninstallApk(string sn, string package)
        {
            string output;
            return AdbShell(sn, "pm uninstall " + package, out output);
        }

        /// <summary>
        /// 安装apk
        /// </summary>
        /// <param name="sn"></param>
        /// <param name="localapk"></param>
        /// <returns></returns>
        public static  bool InstallApk(string sn, string localapk)
        {
            if (!File.Exists(localapk))
                return false;
            string rmtapk = "/data/local/tmp/" + Path.GetFileName(localapk);

            if (Push(sn ,localapk, rmtapk))
            {
                string output;
                AdbShell(sn, "pm install -r -d " + rmtapk, out output);

                Logger.Info("install result:" + output);

                return output.Contains("success");
            }

            return false;
        }

        public static bool Exsit(string sn, string rmt)
        {
            string cmd = "ls " + rmt;
            string o;
            AdbShell(sn, cmd, out o);
            if (o.Contains("No such"))
                return false;
            return true;
        }
        public static bool ApkInstalled(string sn, string package)
        {
            string cmd = "pm path " + package;
            string o;
            AdbShell(sn, cmd, out o);
            return o.Contains(".apk");
        }
		
        static public bool PushToPhone(string sn, string loc, bool force = false)
        {
            if (!File.Exists(loc))
                return false;

            string rmt = "/data/local/tmp/" + Path.GetFileName(loc);

            if (force)
                return Push(sn, loc, rmt);
            else
            {
                if (Exsit(sn, rmt))
                    return true;
                return Push(sn, loc, rmt);
            }
            
        }

        /// <summary>
        /// 推送命令
        /// </summary>
        /// <param name="sn"></param>
        /// <param name="loc"></param>
        /// <param name="rmt"></param>
        /// <returns></returns>
        public static bool Push(string sn, string loc, string rmt)
        {
            if (!File.Exists(loc))
                return false;

            return FileSync.SyncPush(loc, rmt, sn);
        }

        /// <summary>
        /// 获取所有的apk列表
        /// </summary>
        /// <param name="deviceList"></param>
        /// <returns></returns>
        public static bool AdbDevices(out string deviceList)
        {
            AdbClientSock sock = AdbConnectService("host:devices", "");

            deviceList = "";
            if (sock != null)
            {
                byte[] buf = new byte[4]{0, 0, 0, 0};

                int rdlen = 0;
                if (!sock.AdbRead(buf, 4, out rdlen))
                {
                    sock.Close();

                    return false;
                }

                string slen = Encoding.UTF8.GetString(buf);
                int len = Convert.ToInt32(slen, 16);
                byte[] output = new byte[len];

                rdlen = 0;
                if (sock.AdbRead(output, len, out rdlen))
                {
                    deviceList = Encoding.UTF8.GetString(output);
                }
               
                sock.Close();

                return true;
            }
            else
            {
                return false;
            }
        }

        static string Read_And_Dump(AdbClientSock sock)
        {
            byte[] buf = new byte[4096];
            int len = 0;
            
            sock.AdbRead(buf, 4096, out len);

            return Encoding.UTF8.GetString(buf, 0, len);
        }

        public static bool AdbShell(string phoneSn, string cmd,  out string output)
        {
            string msg = "shell:" + cmd;
            AdbClientSock sock = AdbConnectService(msg, phoneSn);

            output = "";
            if (sock != null)
            {
                output = Read_And_Dump(sock);
                return true;
            }

            return false;
        }
        /// <summary>
        /// 连接后台服务
        /// </summary>
        /// <param name="service"></param>
        /// <param name="phoneSn"></param>
        /// <returns></returns>
        public static AdbClientSock AdbConnectService(string service, string phoneSn = "")
        {
            AdbClientSock fd = new AdbClientSock();

            //在我们的应用中,非host服务均针对特定手机
            if (!string.IsNullOrEmpty(phoneSn))
            {
                if (!service.StartsWith("host") && !SwitchAdbTranport(fd, phoneSn))
                {
                    return null;
                }
            }

            if (!SendAdbService(fd, service))
            {
                return null;
            }

            return fd;
        }

        #region 私有域


        /// <summary>
        /// 发送service
        /// </summary>
        /// <param name="sock"></param>
        /// <param name="service"></param>
        /// <returns></returns>
        private static bool SendAdbService(AdbClientSock sock, string service)
        {
            string len = string.Format("{0:x4}", service.Length);

            if (!sock.AdbWrite(len) || !sock.AdbWrite(service))
            {
                sock.Close();
                return false;
            }

            if (!sock.AdbStatus())
            {
                sock.Close();
                return false;
            }

            return true;
        }

        /// <summary>
        /// 非host服务,需要通知
        /// </summary>
        /// <param name="sock"></param>
        /// <param name="phone_sn"></param>
        /// <returns></returns>
        private static bool SwitchAdbTranport(AdbClientSock sock, string phone_sn)
        {
            string service = "host:transport:" + phone_sn;

            return SendAdbService(sock, service);
        }

        #endregion



    }
}
