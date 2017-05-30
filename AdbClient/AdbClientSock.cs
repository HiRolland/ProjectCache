using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

namespace PhoneMgr
{
    /// <summary>
    /// ADB Client Socket 封装
    /// </summary>
    class AdbClientSock
    {

        private Socket _socket = null;

        public AdbClientSock(int port = 5037)
        {
            Create(port);
        }


        ~AdbClientSock()
        {
            Close();
        }

        void Create(int port)
        {
            try
            {
                IPAddress ipAddress = IPAddress.Loopback;
                IPEndPoint ipEndPoint = new IPEndPoint(ipAddress, port);
                _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                _socket.Connect(ipEndPoint);
            }
            catch (Exception)
            {

                //throw;
            }
        }

        public void Close()
        {
            if (_socket != null)
            {
                _socket.Close();
                _socket = null;
            }
        }

        public bool AdbWrite(string val)
        {
            byte[] bs = Encoding.UTF8.GetBytes(val);   //把字符串编码为字节

            return AdbWrite(bs, bs.Length);
        }


        public bool AdbWrite(byte[] val)
        {
            return AdbWrite(val, val.Length);
        }

        /// <summary>
        /// 写入
        /// </summary>
        /// <param name="buf"></param>
        /// <param name="len"></param>
        /// <returns>成功写入len个字节返回true,否则返回false</returns>
        public bool AdbWrite(byte[] buf, int len)
        {

            while (!_socket.Poll(500, SelectMode.SelectWrite)) ;

            int off = 0;
            while (len > 0)
            {
                SocketError err;
                int wr = _socket.Send(buf, off, len, 0, out err);
                if (wr > 0)
                {
                    len -= wr;
                    off += wr;
                }
                else
                {
                    if (wr < 0)
                    {
                        if (err == SocketError.Interrupted)
                            continue;

                        if (err == SocketError.WouldBlock)
                        {
                            Thread.Sleep(1000);
                            continue;
                        }
                    }

                    return false;
                }
            }

            return true;
        }

        public bool AdbRead(byte[] buf, out int rdlen)
        {
            return AdbRead(buf, buf.Length, out rdlen);
        }
        public bool AdbRead(byte[] buf, int expected, out int rdlen)
        {
            rdlen = 0;
            while (!_socket.Poll(50, SelectMode.SelectRead)) ;

            int off = 0;
            while (expected > 0)
            {
                SocketError err;
                int rd = _socket.Receive(buf, off, expected, 0, out err);
                if (rd > 0)
                {
                    expected -= rd;
                    off += rd;

                    rdlen = off;
                }
                else
                {
                    if (rd < 0 && err == SocketError.Interrupted)
                        continue;

                    return false;
                }
            }

            return true;
        }

        public bool AdbStatus()
        {
            int rdlen = 0;
            byte[] buf = new byte[4] { 0, 0, 0, 0 };
            AdbRead(buf, 4, out rdlen);

            string res = Encoding.UTF8.GetString(buf);

            if (res == "OKAY")
                return true;
            else if (res == "FAIL")
                return false;
            else
            {
                return false;
            }
        }

        bool IsConnected()
        {
            return _socket.Connected;
        }

        public bool Connected()
        {
            try
            {
                return !_socket.Poll(5, SelectMode.SelectRead) && (_socket.Available == 0);
            }
            catch (SocketException)
            {
                return false;
            }
            catch (ObjectDisposedException)
            {
                return false;
            }
        }
    }
}
