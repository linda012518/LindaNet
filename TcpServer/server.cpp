#include "TcpServerMgr.hpp"
#include "Config.hpp"

using namespace linda::io;

class MyServer : public TcpServerMgr
{
public:
	MyServer()
	{
		_bSendBack = Config::Instance().hasKey("-sendback");
		_bSendFull = Config::Instance().hasKey("-sendfull");
		_bCheckMsgID = Config::Instance().hasKey("-checkMsgID");
	}
	//cellServer 4 ����̴߳��� ����ȫ
	//���ֻ����1��cellServer���ǰ�ȫ��
	virtual void OnNetJoin(Client* pClient)
	{
		TcpServer::OnNetJoin(pClient);
	}
	//cellServer 4 ����̴߳��� ����ȫ
	//���ֻ����1��cellServer���ǰ�ȫ��
	virtual void OnNetLeave(Client* pClient)
	{
		TcpServer::OnNetLeave(pClient);
	}
	//cellServer 4 ����̴߳��� ����ȫ
	//���ֻ����1��cellServer���ǰ�ȫ��
	virtual void OnNetMsg(Server* pServer, Client* pClient, netmsg_DataHeader* header)
	{
		TcpServer::OnNetMsg(pServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDTHeart();
			netmsg_Login* login = (netmsg_Login*)header;
			//�����ϢID
			if (_bCheckMsgID)
			{
				if (login->msgID != pClient->nRecvMsgID)
				{//��ǰ��ϢID�ͱ�������Ϣ������ƥ��
					CELLLog_Error("OnNetMsg socket<%d> msgID<%d> _nRecvMsgID<%d> %d", pClient->sockfd(), login->msgID, pClient->nRecvMsgID, login->msgID - pClient->nRecvMsgID);
				}
				++pClient->nRecvMsgID;
			}
			//��¼�߼�
			//......
			//��Ӧ��Ϣ
			if (_bSendBack)
			{
				netmsg_LoginR ret;
				ret.msgID = pClient->nSendMsgID;
				if (SOCKET_ERROR == pClient->SendData(&ret))
				{
					//���ͻ��������ˣ���Ϣû����ȥ,Ŀǰֱ��������
					//�ͻ�����Ϣ̫�࣬��Ҫ����Ӧ�Բ���
					//�������ӣ�ҵ��ͻ��˲�������ô����Ϣ
					//ģ�Ⲣ������ʱ�Ƿ���Ƶ�ʹ���
					if (_bSendFull)
					{
						CELLLog_Warring("<Socket=%d> Send Full", pClient->sockfd());
					}
				}
				else {
					++pClient->nSendMsgID;
				}
			}

			//CELLLog_Info("recv <Socket=%d> msgType��CMD_LOGIN, dataLen��%d,userName=%s PassWord=%s", cSock, login->dataLength, login->userName, login->PassWord);
		}//���� ��Ϣ---���� ����   ������ ���ݻ�����  ������ 
		break;
		case CMD_C2S_HEART:
		{
			pClient->resetDTHeart();
			netmsg_s2c_Heart ret;
			pClient->SendData(&ret);
		}
		default:
		{
			CELLLog_Info("recv <socket=%d> undefine msgType,dataLen��%d", pClient->sockfd(), header->dataLength);
		}
		break;
		}
	}
private:
	//�Զ����־ �յ���Ϣ�󽫷���Ӧ����Ϣ
	bool _bSendBack;
	//�Զ����־ �Ƿ���ʾ�����ͻ�������д��
	bool _bSendFull;
	//�Ƿ�����յ�����ϢID�Ƿ�����
	bool _bCheckMsgID;
};

int main(int argc, char* args[])
{
	//����������־����
	Log::Instance().setLogPath("serverLog", "w", false);
	Config::Instance().Init(argc, args);

	const char* strIP = Config::Instance().getStr("strIP", "any");
	uint16_t nPort = Config::Instance().getInt("nPort", 4567);
	int nThread = Config::Instance().getInt("nThread", 1);

	if (Config::Instance().hasKey("-p"))
	{
		CELLLog_Info("hasKey -p");
	}

	if (strcmp(strIP, "any") == 0)
	{
		strIP = nullptr;
	}

	MyServer server;
	if (Config::Instance().hasKey("-ipv6"))
	{
		server.InitSocket(AF_INET6);
	}
	else
	{
		server.InitSocket();
	}

	server.Bind(strIP, nPort);
	server.Listen(SOMAXCONN);
	server.Start(nThread);

	//�����߳��еȴ��û���������
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			server.Close();
			break;
		}
		else {
			CELLLog_Info("undefine cmd");
		}
	}

	CELLLog_Info("exit.");
	return 0;
}
