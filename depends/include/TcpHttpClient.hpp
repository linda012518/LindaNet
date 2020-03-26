#ifndef _TcpHttpClient_HPP_
#define _TcpHttpClient_HPP_

#include <queue>
#include"TcpClientMgr.hpp"
#include "HttpClientC.hpp"

namespace linda {
	namespace io {
		//网络消息接收处理服务类
		class TcpHttpClient :public TcpClientMgr
		{
		public:
			TcpHttpClient()
			{
				NetWork::Init();
			}
		protected:
			virtual Client* makeClientObj(SOCKET cSock, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE)
			{
				return new HttpClientC(cSock, sendSize, recvSize);
			}

		private:
			typedef std::function<void(HttpClientC*)> EventCall;
			struct Event {
				std::string httpUrl;
				EventCall onResponseCall = nullptr;
				bool isGet = true;
				uint8_t questCount = 0;
				uint8_t maxQuestCount = 1;
			};

			std::queue<Event> _eventQueue;

			bool _b_nextRequest = false;
		public:
			bool OnRun(int microseconds = 1)
			{
				if (_b_nextRequest)
					nextRequest();

				return TcpClientMgr::OnRun(microseconds);
			}

			virtual void OnNetMsg(netmsg_DataHeader* header)
			{
				HttpClientC* pHttpClientC = dynamic_cast<HttpClientC*>(_pClient);
				if (!pHttpClientC)
					return;
				if (!pHttpClientC->getResponseInfo())
				{
					return;
				}
				if (_onResponseCall)
				{
					_onResponseCall(pHttpClientC);
					_onResponseCall = nullptr;
				}
				pHttpClientC->onRecvComplete();//接收并且处理完消息调用接收完成事件

				if (!_eventQueue.empty())
					_eventQueue.pop();

				_b_nextRequest = true;
				//nextRequest();
			}

			virtual void OnDisConnect() {
				if (_onResponseCall)
				{
					_onResponseCall(nullptr);
					_onResponseCall = nullptr;
				}

				_b_nextRequest = true;
				//nextRequest();
			};

			void get(const char* url, EventCall onCallback)
			{
				Event e;
				e.httpUrl = url;
				e.onResponseCall = onCallback;
				e.isGet = true;
				_eventQueue.push(e);

				_b_nextRequest = true;

				//if (_onResponseCall)
				//{
				//	//如果正在请求中，添加到队列里
				//	Event e;
				//	e.httpUrl = url;
				//	e.onResponseCall = onCallback;
				//	e.isGet = true;
				//	_eventQueue.push(e);

				//	//_eventQueue.push({ url, onCallback, true });//c++14的写法
				//}
				//else {
				//	_onResponseCall = onCallback;
				//	deatch_http_url(url);
				//	if (0 == hostname2ip(_host.c_str(), _port.c_str()))
				//	{
				//		url2get(_host.c_str(), _path.c_str(), _args.c_str());
				//	}
				//}
			}

			void post(const char* url, EventCall onCallback)
			{
				Event e;
				e.httpUrl = url;
				e.onResponseCall = onCallback;
				e.isGet = false;
				_eventQueue.push(e);

				_b_nextRequest = true;

				//if (_onResponseCall)
				//{
				//	//如果正在请求中，添加到队列里
				//	Event e;
				//	e.httpUrl = url;
				//	e.onResponseCall = onCallback;
				//	e.isGet = false;
				//	_eventQueue.push(e);

				//	//_eventQueue.push({ url, onCallback, false });//c++14特性
				//}
				//else {
				//	_onResponseCall = onCallback;
				//	deatch_http_url(url);
				//	if (0 == hostname2ip(_host.c_str(), _port.c_str()))
				//	{
				//		url2post(_host.c_str(), _path.c_str(), _args.c_str());
				//	}
				//}
			}

			void post(const char* url, const char* dataStr, EventCall onCallback)
			{
				std::string path = url;
				path += '?';
				path += dataStr;
				post(path.c_str(), onCallback);
			}

			int hostname2ip(const char* hostname, const char* port)//域名获取ip
			{
				//char hname[128] = {};
				//gethostname(hname, 127);//获取本机电脑名

				if (!hostname) {
					Log::Error("hostname2ip(hostname is null ptr)");
					return -1;
				}
				if (!port) {
					Log::Error("hostname2ip(port is null ptr)");
					return -1;
				}

				unsigned short p = 80;
				if (port && strlen(port) > 0)
				{
					p = atoi(port);
				}
				//主机和端口号不变，就不重新连接服务器
				if (isRun() && _host_back == _host && _port_back == p)
					return 0;

				addrinfo hints = {};//取出ipv4或ipv6都可以的
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_IP;
				hints.ai_flags = AI_ALL;

				addrinfo* pAddrList = nullptr;
				int ret = getaddrinfo(hostname, nullptr, &hints, &pAddrList);
				if (ret != 0)
				{
					Log::PError("getaddrinfo");
					freeaddrinfo(pAddrList);
					//printf("getaddrinfo error: %s \n", gai_strerror(ret));
					return ret;
				}

				char ipStr[256] = {};
				for (addrinfo* pAddr = pAddrList; pAddr != nullptr; pAddr = pAddr->ai_next)
				{
					ret = getnameinfo(pAddr->ai_addr, pAddr->ai_addrlen, ipStr, sizeof(ipStr) - 1, nullptr, 0, NI_NUMERICHOST);
					if (ret != 0)
					{
						Log::PError("getnameinfo");
						continue;
					}
					else
					{
						if (pAddr->ai_family == AF_INET6) {
							Log::Info("%s ipv6: %s", hostname, ipStr);
						}
						else if (pAddr->ai_family == AF_INET) {
							Log::Info("%s ipv4: %s", hostname, ipStr);
						}
						else {
							Log::Info("%s addr: %s", hostname, ipStr);
						}

						if (connect2ip(pAddr->ai_family, ipStr, p)) {
							_host_back = _host;
							_port_back = p;
							break;
						}
					}
				}

				freeaddrinfo(pAddrList);
				return ret;
			}
		private:
			void nextRequest()
			{
				_b_nextRequest = false;
				if (!_eventQueue.empty())
				{
					Event& e = _eventQueue.front();
					//错误时请求次数大于最大请求数就不要了
					++e.questCount;
					if (e.questCount > e.maxQuestCount) {
						_eventQueue.pop();
						_b_nextRequest = true;
						return;
					}
					//相对即时的
					//if (!_eventQueue.empty())
					//	e = _eventQueue.front();
					//else
					//	return;

					if (e.isGet)
					{
						deatch_http_url(e.httpUrl);
						if (0 == hostname2ip(_host.c_str(), _port.c_str()))
						{
							url2get(_host.c_str(), _path.c_str(), _args.c_str());
							_onResponseCall = e.onResponseCall;
						}
						//get(e.httpUrl.c_str(), e.onResponseCall);
					}
					else
					{
						deatch_http_url(e.httpUrl);
						if (0 == hostname2ip(_host.c_str(), _port.c_str()))
						{
							url2post(_host.c_str(), _path.c_str(), _args.c_str());
							_onResponseCall = e.onResponseCall;
						}
						//post(e.httpUrl.c_str(), e.onResponseCall);
					}
					//_eventQueue.pop();
				}
			}

			void url2get(const char* host, const char* path, const char* args)
			{
				std::string msg = "GET ";
				if (path && strlen(path) > 0) {
					msg += path;
				}
				else {
					msg += "/";
				}
				if (args && strlen(args) > 0) {
					msg += "?";
					msg += args;
				}
				msg += " HTTP/1.1\r\n";

				msg += "Host: ";
				msg += host;
				msg += "\r\n";

				msg += "Connection: keep-alive\r\n";

				msg += "Accpept: */*\r\n";

				msg += "Origin: ";//域
				msg += host;
				msg += "\r\n";

				msg += "\r\n";

				SendData(msg.c_str(), msg.length());
			}

			void url2post(const char* host, const char* path, const char* args)
			{
				std::string msg = "POST ";
				if (path && strlen(path) > 0) {
					msg += path;
				}
				else {
					msg += "/";
				}

				msg += " HTTP/1.1\r\n";

				msg += "Host: ";
				msg += host;
				msg += "\r\n";

				msg += "Connection: keep-alive\r\n";

				msg += "Accpept: */*\r\n";

				msg += "Origin: ";//域
				msg += host;
				msg += "\r\n";

				int bodyLen = 0;
				if (args)
				{
					bodyLen = strlen(args);
				}

				char reqBodyLen[32] = {};
				sprintf(reqBodyLen, "Content-Length: %d\r\n", bodyLen);
				msg += reqBodyLen;

				msg += "\r\n";

				if (bodyLen > 0) {
					msg += args;
				}

				SendData(msg.c_str(), msg.length());
			}

			bool connect2ip(int af, const char* ip, unsigned short port)
			{
				bool result = false;
				do
				{
					if (!ip) break;

					if (INVALID_SOCKET == InitSocket(af, 10240, 102400))
						break;
					if (SOCKET_ERROR == Connect(ip, port))
						break;;
					Log::Info("connect2ip(%s, %d)", ip, port);
					result = true;

				} while (false);

				return result;


				//if (!ip)
				//	return false;
				//unsigned short p = 80;
				//if (port && strlen(port) > 0)
				//{
				//	p = atoi(port);
				//}
				//if (SOCKET_ERROR == Connect(ip, p))
				//	return false;
				//Log::Info("connect2ip(%s, %d)", ip, p);
				//return true;
			}

			void deatch_http_url(std::string httpurl)
			{
				_httpType.clear();
				_host.clear();
				_port.clear();
				_path.clear();
				_args.clear();

				auto pos1 = httpurl.find("://");
				if (pos1 != std::string::npos)
				{
					_httpType = httpurl.substr(0, pos1);
					pos1 += 3;
				}
				else
				{
					_httpType = "https";
					pos1 = 0;
				}

				auto pos2 = httpurl.find('/', pos1);
				if (pos2 != std::string::npos)
				{
					_host = httpurl.substr(pos1, pos2 - pos1);
					_path = httpurl.substr(pos2);

					pos1 = _path.find('?');
					if (pos1 != std::string::npos)
					{
						_args = _path.substr(pos1 + 1);
						_path = _path.substr(0, pos1);
					}
				}
				else
				{
					_host = httpurl.substr(pos1);
				}

				pos1 = _host.find(':');
				if (pos1 != std::string::npos)
				{
					_port = _host.substr(pos1 + 1);
					_host = _host.substr(0, pos1);
				}
			}
		private:
			std::string _httpType;
			std::string _host;
			std::string _port;
			std::string _path;
			std::string _args;

			std::string _host_back;
			unsigned short _port_back;

			EventCall _onResponseCall = nullptr;
		};
	}
}
#endif // !_TcpHttpClient_HPP_
