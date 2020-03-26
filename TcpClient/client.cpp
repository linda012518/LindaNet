#include "TcpClientMgr.hpp"
#include"Config.hpp"
#include"Thread.hpp"

#include<atomic>
#include<list>
#include<vector>
using namespace std;
using namespace linda::io;

//�����IP��ַ
const char * strIP = "127.0.0.1";
//����˶˿�
uint16_t nPort = 4567;
//�����߳�����
int nThread = 1;
//�ͻ�������
int nClient = 1;
/*
::::::���ݻ���д�뷢�ͻ�����
::::::�ȴ�socket��дʱ��ʵ�ʷ���
::ÿ���ͻ�����nSendSleep(����)ʱ����
::����д��nMsg��Login��Ϣ
::ÿ����Ϣ100�ֽڣ�Login��
*/
//�ͻ���ÿ�η�������Ϣ
int nMsg = 1;
//д����Ϣ���������ļ��ʱ��
int nSendSleep = 1;
//��������ʱ��
int nWorkSleep = 1;
//�ͻ��˷��ͻ�������С
int nSendBuffSize = SEND_BUFF_SZIE;
//�ͻ��˽��ջ�������С
int nRecvBuffSize = RECV_BUFF_SZIE;
//�Ƿ���-���͵������ѱ���������Ӧ
int bChekSendBack = true;

class MyClient : public TcpClientMgr
{
public:
	MyClient()
	{
		_bCheckMsgID = Config::Instance().hasKey("-checkMsgID");
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(netmsg_DataHeader* header)
	{
		_bSend = false;
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			netmsg_LoginR* login = (netmsg_LoginR*)header;
			if (_bCheckMsgID)
			{
				if (login->msgID != _nRecvMsgID)
				{//��ǰ��ϢID�ͱ�������Ϣ������ƥ��
					CELLLog_Error("OnNetMsg socket<%d> msgID<%d> _nRecvMsgID<%d> %d", _pClient->sockfd(), login->msgID, _nRecvMsgID, login->msgID - _nRecvMsgID);
				}
				++_nRecvMsgID;
			}
			//CELLLog_Info("<socket=%d> recv msgType��CMD_LOGIN_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			netmsg_LogoutR* logout = (netmsg_LogoutR*)header;
			//CELLLog_Info("<socket=%d> recv msgType��CMD_LOGOUT_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
			//CELLLog_Info("<socket=%d> recv msgType��CMD_NEW_USER_JOIN", (int)_pClient->sockfd());
		}
		break;
		case CMD_ERROR:
		{
			CELLLog_Info("<socket=%d> recv msgType��CMD_ERROR", (int)_pClient->sockfd());
		}
		break;
		default:
		{
			CELLLog_Info("error, <socket=%d> recv undefine msgType", (int)_pClient->sockfd());
		}
		}
	}

	int SendTest(netmsg_Login* login)
	{
		int ret = 0;
		//���ʣ�෢�ʹ�������0
		if (_nSendCount > 0 && !_bSend)
		{
			login->msgID = _nSendMsgID;
			ret = SendData(login);
			//CELLLog_Info("%d", _nSendMsgID);
			if (SOCKET_ERROR != ret)
			{
				_bSend = bChekSendBack;
				++_nSendMsgID;
				//���ʣ�෢�ʹ�������һ��
				--_nSendCount;
			}
		}
		return ret;
	}

	bool checkSend(time_t dt)
	{
		_tRestTime += dt;
		//ÿ����nSendSleep����
		if (_tRestTime >= nSendSleep)
		{
			//���ü�ʱ
			_tRestTime -= nSendSleep;
			//���÷��ͼ���
			_nSendCount = nMsg;
		}
		return _nSendCount > 0;
	}
public:
	//����ʱ�����
	time_t _tRestTime = 0;
private:
	//������Ϣid����
	int _nRecvMsgID = 1;
	//������Ϣid����
	int _nSendMsgID = 1;
	//������������
	int _nSendCount = 0;
	//�����յ��ķ������ϢID�Ƿ�����
	bool _bCheckMsgID = false;
	//
	bool _bSend = false;
};

std::atomic_int sendCount(0);
std::atomic_int readyCount(0);
std::atomic_int nConnect(0);

void WorkThread(Thread* pThread, int id)
{
	//n���߳� idֵΪ 1~n
	CELLLog_Info("thread<%d>,start", id);
	//�ͻ�������
	vector<MyClient*> clients(nClient);
	//���㱾�߳̿ͻ�����clients�ж�Ӧ��index
	int begin = 0;
	int end = nClient / nThread;
	if (end < 1)
		end = 1;
	int nTemp1 = nSendSleep > 0 ? nSendSleep : 1;
	for (int n = begin; n < end; n++)
	{
		if (!pThread->isRun())
			break;
		clients[n] = new MyClient();
		clients[n]->_tRestTime = n % nTemp1;
		//���߳�ʱ����CPU
		Thread::Sleep(0);
	}
	for (int n = begin; n < end; n++)
	{
		if (!pThread->isRun())
			break;
		if (Config::Instance().hasKey("-ipv6"))
		{
			clients[n]->setScopeIdName(Config::Instance().getStr("scope_id_name", ""));
			if (INVALID_SOCKET == clients[n]->InitSocket(AF_INET6, nSendBuffSize, nRecvBuffSize))
				break;
		}
		else
		{
			if (INVALID_SOCKET == clients[n]->InitSocket(AF_INET, nSendBuffSize, nRecvBuffSize))
				break;
		}
		if (SOCKET_ERROR == clients[n]->Connect(strIP, nPort))
			break;
		nConnect++;
		Thread::Sleep(0);
	}
	//�����������
	CELLLog_Info("thread<%d>,Connect<begin=%d, end=%d ,nConnect=%d>", id, begin, end, (int)nConnect);

	readyCount++;
	while (readyCount < nThread && pThread->isRun())
	{//�ȴ������߳�׼���ã��ٷ�������
		Thread::Sleep(10);
	}

	//��Ϣ
	netmsg_Login login;
	//�����������ֵ
	strcpy(login.userName, "lyd");
	strcpy(login.PassWord, "lydmm");
	//
	//�շ����ݶ���ͨ��onRun�߳�
	//SendDataֻ�ǽ�����д�뷢�ͻ�����
	//�ȴ�select����дʱ�Żᷢ������
	//�ɵ�ʱ���
	auto t2 = Time::getNowInMilliSec();
	//�µ�ʱ���
	auto t0 = t2;
	//������ʱ��
	auto dt = t0;
	Timestamp tTime;
	while (pThread->isRun())
	{
		t0 = Time::getNowInMilliSec();
		dt = t0 - t2;
		t2 = t0;
		//����while (pThread->isRun())ѭ����Ҫ��������
		//����work
		{
			int count = 0;
			//ÿ��ÿ���ͻ��˷���nMsg������
			for (int m = 0; m < nMsg; m++)
			{
				//ÿ���ͻ���1��1����д����Ϣ
				for (int n = begin; n < end; n++)
				{
					if (clients[n]->isRun())
					{
						if (clients[n]->SendTest(&login) > 0)
						{
							++sendCount;
						}
					}
				}
			}
			//sendCount += count;
			for (int n = begin; n < end; n++)
			{
				if (clients[n]->isRun())
				{	//��ʱ����Ϊ0��ʾselect���״̬����������
					if (!clients[n]->OnRun(0))
					{	//���ӶϿ�
						nConnect--;
						continue;
					}
					//��ⷢ�ͼ����Ƿ���Ҫ����
					clients[n]->checkSend(dt);
				}
			}
		}
		Thread::Sleep(nWorkSleep);
	}
	//--------------------------
	//�ر���Ϣ�շ��߳�
	//tRun.Close();
	//�رտͻ���
	for (int n = begin; n < end; n++)
	{
		clients[n]->Close();
		delete clients[n];
	}
	CELLLog_Info("thread<%d>,exit", id);
	--readyCount;
}

int main(int argc, char *args[])
{
	//����������־����
	Log::Instance().setLogPath("clientLog", "w", false);

	Config::Instance().Init(argc, args);

	strIP = Config::Instance().getStr("strIP", "127.0.0.1");
	nPort = Config::Instance().getInt("nPort", 4567);
	nThread = Config::Instance().getInt("nThread", 1);
	nClient = Config::Instance().getInt("nClient", 10000);
	nMsg = Config::Instance().getInt("nMsg", 10);
	nSendSleep = Config::Instance().getInt("nSendSleep", 100);
	nWorkSleep = Config::Instance().getInt("nWorkSleep", 1);
	bChekSendBack = Config::Instance().hasKey("-chekSendBack");
	nSendBuffSize = Config::Instance().getInt("nSendBuffSize", SEND_BUFF_SZIE);
	nRecvBuffSize = Config::Instance().getInt("nRecvBuffSize", RECV_BUFF_SZIE);

	//�����ն������߳�
	//���ڽ�������ʱ�û������ָ��
	Thread tCmd;
	tCmd.Start(nullptr, [](Thread* pThread) {
		while (true)
		{
			char cmdBuf[256] = {};
			scanf("%s", cmdBuf);
			if (0 == strcmp(cmdBuf, "exit"))
			{
				//pThread->Exit();
				CELLLog_Info("�˳�cmdThread�߳�");
				break;
			}
			else {
				CELLLog_Info("��֧�ֵ����");
			}
		}
	});

	//����ģ��ͻ����߳�
	vector<Thread*> threads;
	for (int n = 0; n < nThread; n++)
	{
		Thread* t = new Thread();
		t->Start(nullptr, [n](Thread* pThread) {
			WorkThread(pThread, n + 1);
		});
		threads.push_back(t);
	}
	//ÿ������ͳ��
	Timestamp tTime;
	while (tCmd.isRun())
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			CELLLog_Info("thread<%d>,clients<%d>,connect<%d>,time<%lf>,send<%d>", nThread, nClient, (int)nConnect, t, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		Thread::Sleep(1);
	}
	//
	for (auto t : threads)
	{
		t->Close();
		delete t;
	}
	CELLLog_Info("�������˳�����");
	return 0;
}
