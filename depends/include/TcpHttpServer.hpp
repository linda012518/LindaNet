#ifndef _TcpHttpServer_HPP_
#define _TcpHttpServer_HPP_

#include"TcpServerMgr.hpp"
#include"HttpClientS.hpp"

namespace linda {
	namespace io {
		//网络消息接收处理服务类
		class TcpHttpServer :public TcpServerMgr
		{
			virtual Client* makeClientObj(SOCKET cSock)
			{
				return new HttpClientS(cSock, _nSendBuffSize, _nRecvBuffSize);
			}

			virtual void OnNetMsg(Server* pServer, Client* pClient, netmsg_DataHeader* header)
			{

				HttpClientS* httpClient = (HttpClientS*)pClient;
				if (!httpClient)
					return;
				if (!httpClient->getRequestInfo())
					return;

				TcpServer::OnNetMsg(pServer, pClient, header);
				httpClient->resetDTHeart();

				OnNetMsgHttp(pServer, (HttpClientS*)pClient);
			}

			virtual void OnNetMsgHttp(Server* pServer, HttpClientS* httpClient)
			{

			}
		};
	}
}
#endif // !_TcpHttpServer_HPP_
