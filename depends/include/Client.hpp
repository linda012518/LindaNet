#ifndef _CELLClient_HPP_
#define _CELLClient_HPP_

#include"CELL.hpp"
#include"Buffer.hpp"
#include"NetWork.hpp"

namespace linda {
	namespace io {
		//客户端心跳检测死亡计时时间
#define CLIENT_HREAT_DEAD_TIME 5000
//在间隔指定时间后才允许发送
#define CLIENT_SEND_BUFF_TIME 200

		enum ClientState 
		{
			clientState_create = 10,
			clientState_join,
			clientState_run,
			clientState_close,
		};
//客户端数据类型
		class Client
		{
			//////////用于调试的成员变量
		public:
			int id = -1;
			//所属serverid
			int serverId = -1;
			//测试接收发逻辑用
			//用于server检测接收到的消息ID是否连续
			int nRecvMsgID = 1;
			//测试接收发逻辑用
			//用于client检测接收到的消息ID是否连续
			int nSendMsgID = 1;
			///////////////////////////////////
		public:
			Client(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
				_sendBuff(sendSize),
				_recvBuff(recvSize)
			{
				static int n = 1;
				id = n++;
				_sockfd = sockfd;

				resetDTHeart();
				resetDTSend();
			}

			virtual ~Client()
			{
				//CELLLog_Info("~Client[sId=%d id=%d socket=%d]", serverId, id, (int)_sockfd);
				destorySocket();
			}

			void destorySocket()
			{
				if (INVALID_SOCKET != _sockfd)
				{
					//CELLLog_Info("Client::destory[sId=%d id=%d socket=%d]", serverId, id, (int)_sockfd);
					NetWork::destorySocket(_sockfd);
					_sockfd = INVALID_SOCKET;
				}
			}

			SOCKET sockfd()
			{
				return _sockfd;
			}

			int RecvData()
			{
				return _recvBuff.read4socket(_sockfd);
			}

			virtual bool hasMsg()
			{
				return _recvBuff.hasMsg();
			}

			netmsg_DataHeader* front_msg()
			{
				return (netmsg_DataHeader*)_recvBuff.data();
			}

			virtual void pop_front_msg()
			{
				if (hasMsg())
					_recvBuff.pop(front_msg()->dataLength);
			}

			bool needWrite()
			{
				return _sendBuff.needWrite();
			}

			//立即将发送缓冲区的数据发送给客户端
			int SendDataReal()
			{
				resetDTSend();
				int ret = _sendBuff.write2socket(_sockfd);
				if (_sendBuff.dataLen() == 0)
				{
					onSendComplete();
				}
				return ret;
			}

			virtual void onSendComplete() { }

			//缓冲区的控制根据业务需求的差异而调整
			//发送数据
			int SendData(netmsg_DataHeader* header)
			{
				return SendData((const char*)header, header->dataLength);
			}

			int SendData(const char* pData, int len)
			{
				if (_sendBuff.push(pData, len))
				{
					return len;
				}
				return SOCKET_ERROR;
			}

			void resetDTHeart()
			{
				_dtHeart = 0;
			}

			void resetDTSend()
			{
				_dtSend = 0;
			}

			//心跳检测
			bool checkHeart(time_t dt)
			{
				if (isClose())
					return true;
				_dtHeart += dt;
				if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
				{
					CELLLog_Info("checkHeart dead:s=%d,time=%ld", _sockfd, _dtHeart);
					return true;
				}
				return false;
			}

			//定时发送消息检测
			bool checkSend(time_t dt)
			{
				_dtSend += dt;
				if (_dtSend >= CLIENT_SEND_BUFF_TIME)
				{
					//CELLLog_Info("checkSend:s=%d,time=%d", _sockfd, _dtSend);
					//立即将发送缓冲区的数据发送出去
					SendDataReal();
					//重置发送计时
					resetDTSend();
					return true;
				}
				return false;
			}

			void setIP(char* ip)
			{
				if (ip)
					strncpy(_ip, ip, INET6_ADDRSTRLEN);
			}

			char* getIP()
			{
				return _ip;
			}

			ClientState state()
			{
				return _clientState;
			}

			void state(ClientState state)
			{
				_clientState = state;
			}

			void onClose()
			{
				CELLLog_Info("sockfd<%d> onClose", _sockfd);
				state(clientState_close);
			}

			bool isClose()
			{
				return _clientState == ClientState::clientState_close;
			}

		protected:
			// socket fd_set  file desc set
			SOCKET _sockfd = INVALID_SOCKET;
			//第二缓冲区 接收消息缓冲区
			Buffer _recvBuff;
			//发送缓冲区
			Buffer _sendBuff;
			//心跳死亡计时
			time_t _dtHeart = 0;
			//上次发送消息数据的时间 
			time_t _dtSend = 0;
			char _ip[INET6_ADDRSTRLEN] = {};
			ClientState _clientState = ClientState::clientState_create;
		};

	}
}
#endif // !_CELLClient_HPP_