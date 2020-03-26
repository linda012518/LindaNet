#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#include"CELL.hpp"
#include"NetWork.hpp"
#include"MessageHeader.hpp"
#include"Client.hpp"
#include<string>

namespace linda {
	namespace io {
		class TcpClient
		{
		public:
			TcpClient()
			{
				_isConnect = false;
			}

			virtual ~TcpClient()
			{
				Close();
			}
			//初始化socket
			SOCKET InitSocket(int netVersion, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE)
			{
				NetWork::Init();

				if (_pClient)
				{
					CELLLog_Info("warning, initSocket close old socket<%d>...", (int)_pClient->sockfd());
					Close();
				}
				_address_family = netVersion;
				SOCKET sock = socket(_address_family, SOCK_STREAM, IPPROTO_TCP);
				if (INVALID_SOCKET == sock)
				{
					CELLLog_PError("create socket failed...");
				}
				else {
					NetWork::make_reuseaddr(sock);
					//CELLLog_Info("create socket<%d> success...", (int)sock);
					_pClient = makeClientObj(sock, sendSize, recvSize);
					OnInitSocket();
				}
				return sock;
			}

			//连接服务器
			int Connect(const char* ip, unsigned short port)
			{
				if (!_pClient)
				{
					return SOCKET_ERROR;
				}
				int ret = SOCKET_ERROR;
				if (_address_family == AF_INET)
				{
					// 2 连接服务器 connect
					sockaddr_in _sin = {};
					_sin.sin_family = AF_INET;
					_sin.sin_port = htons(port);
#ifdef _WIN32
					_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
					_sin.sin_addr.s_addr = inet_addr(ip);
#endif
					//CELLLog_Info("<socket=%d> connecting <%s:%d>...", (int)_pClient->sockfd(), ip, port);
					ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
				}
				else
				{
					sockaddr_in6 _sin = {};
					_sin.sin6_family = AF_INET6;
					_sin.sin6_port = htons(port);
					//指定网络接口名称字符串作为参数；若该接口存在，则返回相应的索引，否则返回0
					_sin.sin6_scope_id = if_nametoindex(_scope_id_name.c_str());
					inet_pton(AF_INET6, ip, &_sin.sin6_addr);

					//CELLLog_Info("<socket=%d> connecting <%s:%d>...", (int)_pClient->sockfd(), ip, port);
					ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in6));
				}

				if (SOCKET_ERROR == ret)
				{
					CELLLog_PError("<socket=%d> connect <%s:%d> failed...", (int)_pClient->sockfd(), ip, port);
				}
				else {
					_isConnect = true;
					OnConnect();
					//CELLLog_Info("<socket=%d> connect <%s:%d> success...", (int)_pClient->sockfd(), ip, port);
				}
				return ret;
			}

			//关闭套节字closesocket
			virtual void Close()
			{
				if (_pClient)
				{
					delete _pClient;
					_pClient = nullptr;
					_isConnect = false;
					OnDisConnect();
				}
			}

			//处理网络消息
			virtual bool OnRun(int microseconds = 1) = 0;

			//是否工作中
			bool isRun()
			{
				return _pClient && _isConnect;
			}

			//接收数据 处理粘包 拆分包
			int RecvData()
			{
				if (isRun())
				{
					//接收客户端数据
					int nLen = _pClient->RecvData();
					if (nLen > 0)
					{
						DoMsg();
					}
					return nLen;
				}
				return 0;
			}

			void DoMsg()
			{
				//循环 判断是否有消息需要处理
				while (_pClient->hasMsg())
				{
					//处理网络消息
					OnNetMsg(_pClient->front_msg());
					if (_pClient->isClose())
					{
						Close();
						return;
					}
					//移除消息队列（缓冲区）最前的一条数据
					_pClient->pop_front_msg();

				}
			}

			//响应网络消息
			virtual void OnNetMsg(netmsg_DataHeader* header) = 0;

			//发送数据
			int SendData(netmsg_DataHeader* header)
			{
				if (isRun())
					return _pClient->SendData(header);
				return SOCKET_ERROR;
			}

			int SendData(const char* pData, int len)
			{
				if (isRun())
					return _pClient->SendData(pData, len);
				return SOCKET_ERROR;
			}

			void setScopeIdName(std::string scopeid)
			{
				_scope_id_name = scopeid;
			}
		protected:
			virtual Client* makeClientObj(SOCKET cSock, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE)
			{
				return new Client(cSock, sendSize, recvSize);
			}

			virtual void OnInitSocket() {

			};

			virtual void OnConnect() {

			};

			virtual void OnDisConnect() {

			};
		protected:
			Client* _pClient = nullptr;
			int _address_family = AF_INET;
			//设置IPv6地址作用范围，是可聚集的全球化地址，还是本地站地址，还是链路地址
			std::string _scope_id_name;//网络接口名称字符串
			bool _isConnect = false;
		};
	}
}
#endif