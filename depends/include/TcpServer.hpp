#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#include"CELL.hpp"
#include"Client.hpp"
#include"Server.hpp"
#include"INetEvent.hpp"
#include"NetWork.hpp"
#include"Config.hpp"

#include<thread>
#include<mutex>
#include<atomic>

namespace linda {
	namespace io {
		class TcpServer : public INetEvent
		{
		private:
			//
			Thread _thread;
			//消息处理对象，内部会创建线程
			std::vector<Server*> _cellServers;
			//每秒消息计时
			Timestamp _tTime;
			//
			SOCKET _sock;
		protected:
			int _address_family = AF_INET;
			//客户端发送缓冲区大小
			int _nSendBuffSize;
			//客户端接收缓冲区大小
			int _nRecvBuffSize;
			//客户端连接上限
			int _nMaxClient;
			//SOCKET recv计数
			std::atomic_int _recvCount;
			//收到消息计数
			std::atomic_int _msgCount;
			//客户端计数
			std::atomic_int _clientAccept;
			//已分配客户端计数
			std::atomic_int _clientJoin;
		public:
			TcpServer()
			{
				_sock = INVALID_SOCKET;
				_recvCount = 0;
				_msgCount = 0;
				_clientAccept = 0;
				_clientJoin = 0;
				_nSendBuffSize = Config::Instance().getInt("nSendBuffSize", SEND_BUFF_SZIE);
				_nRecvBuffSize = Config::Instance().getInt("nRecvBuffSize", RECV_BUFF_SZIE);
				_nMaxClient = Config::Instance().getInt("nMaxClient", FD_SETSIZE);
			}
			virtual ~TcpServer()
			{
				Close();
			}
			//初始化Socket
			SOCKET InitSocket(int netVersion = AF_INET)
			{
				NetWork::Init();
				if (INVALID_SOCKET != _sock)
				{
					CELLLog_Warring("initSocket close old socket<%d>...", (int)_sock);
					Close();
				}
				_address_family = netVersion;
				_sock = socket(_address_family, SOCK_STREAM, IPPROTO_TCP);
				if (INVALID_SOCKET == _sock)
				{
					CELLLog_PError("create socket failed...");
				}
				else {
					NetWork::make_reuseaddr(_sock);
					CELLLog_Info("create socket<%d> success...", (int)_sock);
				}
				return _sock;
			}

			//绑定IP和端口号
			int Bind(const char* ip, unsigned short port)
			{
				int ret = SOCKET_ERROR;
				// 2 bind 绑定用于接受客户端连接的网络端口
				if (AF_INET == _address_family)
				{
					sockaddr_in _sin = {};
					_sin.sin_family = AF_INET;
					_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
					if (ip) {
						_sin.sin_addr.S_un.S_addr = inet_addr(ip);
					}
					else {
						_sin.sin_addr.S_un.S_addr = INADDR_ANY;
					}
#else
					if (ip) {
						_sin.sin_addr.s_addr = inet_addr(ip);
					}
					else {
						_sin.sin_addr.s_addr = INADDR_ANY;
					}
#endif
					ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
				}
				else if(AF_INET6 == _address_family)
				{
					sockaddr_in6 _sin = {};
					_sin.sin6_family = AF_INET6;
					_sin.sin6_port = htons(port);//host to net unsigned short

					if (ip) {
						inet_pton(AF_INET6, ip, &_sin.sin6_addr);
					}
					else {
						_sin.sin6_addr = in6addr_any;
					}
					ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
				}

				if (SOCKET_ERROR == ret)
				{
					CELLLog_PError("bind port<%d> failed...", port);
				}
				else {
					CELLLog_Info("bind port<%d> success...", port);
				}
				return ret;
			}

			//监听端口号
			int Listen(int n)
			{
				// 3 listen 监听网络端口
				int ret = listen(_sock, n);
				if (SOCKET_ERROR == ret)
				{
					CELLLog_PError("listen socket<%d> failed...", _sock);
				}
				else {
					CELLLog_Info("listen port<%d> success...", _sock);
				}
				return ret;
			}

			//接受客户端连接
			SOCKET Accept()
			{
				if (_address_family == AF_INET)
				{
					return Accept_IPv4();
				}
				else
				{
					return Accept_IPv6();
				}
			}

			SOCKET Accept_IPv6()
			{
				// 4 accept 等待接受客户端连接
				sockaddr_in6 clientAddr = {};
				int nAddrLen = sizeof(clientAddr);
				SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
				cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
				cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
				if (INVALID_SOCKET == cSock)
				{
					CELLLog_PError("accept INVALID_SOCKET...");
				}
				else
				{
					//获取IP地址 CELLLog_Info是跨线程了，ip可能会释放
					static char ip[INET6_ADDRSTRLEN] = {};
					inet_ntop(AF_INET6, &clientAddr.sin6_addr, ip, INET6_ADDRSTRLEN - 1);
					AcceptClient(cSock, ip);
				}
				return cSock;
			}

			SOCKET Accept_IPv4()
			{
				// 4 accept 等待接受客户端连接
				sockaddr_in clientAddr = {};
				int nAddrLen = sizeof(sockaddr_in);
				SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
				cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
				cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
				if (INVALID_SOCKET == cSock)
				{
					CELLLog_PError("accept INVALID_SOCKET...");
				}
				else
				{
					//获取IP地址
					char* ip = inet_ntoa(clientAddr.sin_addr);
					AcceptClient(cSock, ip);
				}
				return cSock;
			}

			virtual Client* makeClientObj(SOCKET cSock)
			{
				return new Client(cSock, _nSendBuffSize, _nRecvBuffSize);
			}

			void AcceptClient(SOCKET cSock, char* ip)
			{
				NetWork::make_reuseaddr(cSock);//端口不能用的情况
				CELLLog_Info("AcceptEx IP: %s, %d", ip, cSock);
				if (_clientAccept < _nMaxClient)
				{
					_clientAccept++;
					//将新客户端分配给客户数量最少的cellServer
					auto c = makeClientObj(cSock);
					c->setIP(ip);
					addClientToCELLServer(c);
				}
				else {
					//获取IP地址 inet_ntoa(clientAddr.sin_addr)
					NetWork::destorySocket(cSock);
					CELLLog_Warring("Accept to nMaxClient");
				}
			}

			void addClientToCELLServer(Client* pClient)
			{
				//查找客户数量最少的Server消息处理对象
				auto pMinServer = _cellServers[0];
				for (auto pServer : _cellServers)
				{
					if (pMinServer->getClientCount() > pServer->getClientCount())
					{
						pMinServer = pServer;
					}
				}
				pMinServer->addClient(pClient);
			}

			template<class ServerT>
			void Start(int nCELLServer)
			{
				for (int n = 0; n < nCELLServer; n++)
				{
					auto ser = new ServerT();
					ser->setId(n + 1);
					ser->setClientNum((_nMaxClient / nCELLServer) + 1);
					_cellServers.push_back(ser);
					//注册网络事件接受对象
					ser->setEventObj(this);
					//启动消息处理线程
					ser->Start();
				}
				_thread.Start(nullptr,
					[this](Thread* pThread) {
					OnRun(pThread);
				});
			}
			//关闭Socket
			void Close()
			{
				CELLLog_Info("TcpServer.Close begin");
				_thread.Close();
				if (_sock != INVALID_SOCKET)
				{
					for (auto s : _cellServers)
					{
						delete s;
					}
					_cellServers.clear();
					//关闭套节字socket
					NetWork::destorySocket(_sock);
					_sock = INVALID_SOCKET;
				}
				CELLLog_Info("TcpServer.Close end");
			}

			//cellServer 4 多个线程触发 不安全
			//如果只开启1个cellServer就是安全的
			virtual void OnNetJoin(Client* pClient)
			{
				_clientJoin++;
				//CELLLog_Info("client<%d> join", pClient->sockfd());
			}
			//cellServer 4 多个线程触发 不安全
			//如果只开启1个cellServer就是安全的
			virtual void OnNetLeave(Client* pClient)
			{
				_clientAccept--;
				_clientJoin--;
				CELLLog_Info("client<%d> leave", pClient->sockfd());
			}
			//cellServer 4 多个线程触发 不安全
			//如果只开启1个cellServer就是安全的
			virtual void OnNetMsg(Server* pServer, Client* pClient, netmsg_DataHeader* header)
			{
				_msgCount++;
			}

			virtual void OnNetRecv(Client* pClient)
			{
				_recvCount++;
				//CELLLog_Info("client<%d> leave", pClient->sockfd());
			}
		protected:
			//处理网络消息
			virtual void OnRun(Thread* pThread) = 0;

			//计算并输出每秒收到的网络消息
			void time4msg()
			{
				auto t1 = _tTime.getElapsedSecond();
				if (t1 >= 1.0)
				{
					CELLLog_Info("thread<%d>,time<%lf>,socket<%d>,Accept<%d>,Join<%d>,recv<%d>,msg<%d>"
						, (int)_cellServers.size()
						, t1, _sock
						, (int)_clientAccept, (int)_clientJoin
						, (int)_recvCount, (int)_msgCount);
					_recvCount = 0;
					_msgCount = 0;
					_tTime.update();
				}
			}

			SOCKET sockfd()
			{
				return _sock;
			}
		};
	}
}
#endif // !_EasyTcpServer_hpp_
