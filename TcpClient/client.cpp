#include "TcpClientMgr.hpp"
#include"Config.hpp"
#include"Thread.hpp"

#include<atomic>
#include<list>
#include<vector>
using namespace std;
using namespace linda::io;

//服务端IP地址
const char * strIP = "127.0.0.1";
//服务端端口
uint16_t nPort = 4567;
//发送线程数量
int nThread = 1;
//客户端数量
int nClient = 1;
/*
::::::数据会先写入发送缓冲区
::::::等待socket可写时才实际发送
::每个客户端在nSendSleep(毫秒)时间内
::最大可写入nMsg条Login消息
::每条消息100字节（Login）
*/
//客户端每次发几条消息
int nMsg = 1;
//写入消息到缓冲区的间隔时间
int nSendSleep = 1;
//工作休眠时间
int nWorkSleep = 1;
//客户端发送缓冲区大小
int nSendBuffSize = SEND_BUFF_SZIE;
//客户端接收缓冲区大小
int nRecvBuffSize = RECV_BUFF_SZIE;
//是否检测-发送的请求已被服务器回应
int bChekSendBack = true;

class MyClient : public TcpClientMgr
{
public:
	MyClient()
	{
		_bCheckMsgID = Config::Instance().hasKey("-checkMsgID");
	}
	//响应网络消息
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
				{//当前消息ID和本地收消息次数不匹配
					CELLLog_Error("OnNetMsg socket<%d> msgID<%d> _nRecvMsgID<%d> %d", _pClient->sockfd(), login->msgID, _nRecvMsgID, login->msgID - _nRecvMsgID);
				}
				++_nRecvMsgID;
			}
			//CELLLog_Info("<socket=%d> recv msgType：CMD_LOGIN_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			netmsg_LogoutR* logout = (netmsg_LogoutR*)header;
			//CELLLog_Info("<socket=%d> recv msgType：CMD_LOGOUT_RESULT", (int)_pClient->sockfd());
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
			//CELLLog_Info("<socket=%d> recv msgType：CMD_NEW_USER_JOIN", (int)_pClient->sockfd());
		}
		break;
		case CMD_ERROR:
		{
			CELLLog_Info("<socket=%d> recv msgType：CMD_ERROR", (int)_pClient->sockfd());
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
		//如果剩余发送次数大于0
		if (_nSendCount > 0 && !_bSend)
		{
			login->msgID = _nSendMsgID;
			ret = SendData(login);
			//CELLLog_Info("%d", _nSendMsgID);
			if (SOCKET_ERROR != ret)
			{
				_bSend = bChekSendBack;
				++_nSendMsgID;
				//如果剩余发送次数减少一次
				--_nSendCount;
			}
		}
		return ret;
	}

	bool checkSend(time_t dt)
	{
		_tRestTime += dt;
		//每经过nSendSleep毫秒
		if (_tRestTime >= nSendSleep)
		{
			//重置计时
			_tRestTime -= nSendSleep;
			//重置发送计数
			_nSendCount = nMsg;
		}
		return _nSendCount > 0;
	}
public:
	//发送时间计数
	time_t _tRestTime = 0;
private:
	//接收消息id计数
	int _nRecvMsgID = 1;
	//发送消息id计数
	int _nSendMsgID = 1;
	//发送条数计数
	int _nSendCount = 0;
	//检查接收到的服务端消息ID是否连续
	bool _bCheckMsgID = false;
	//
	bool _bSend = false;
};

std::atomic_int sendCount(0);
std::atomic_int readyCount(0);
std::atomic_int nConnect(0);

void WorkThread(Thread* pThread, int id)
{
	//n个线程 id值为 1~n
	CELLLog_Info("thread<%d>,start", id);
	//客户端数组
	vector<MyClient*> clients(nClient);
	//计算本线程客户端在clients中对应的index
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
		//多线程时让下CPU
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
	//所有连接完成
	CELLLog_Info("thread<%d>,Connect<begin=%d, end=%d ,nConnect=%d>", id, begin, end, (int)nConnect);

	readyCount++;
	while (readyCount < nThread && pThread->isRun())
	{//等待其它线程准备好，再发送数据
		Thread::Sleep(10);
	}

	//消息
	netmsg_Login login;
	//给点有意义的值
	strcpy(login.userName, "lyd");
	strcpy(login.PassWord, "lydmm");
	//
	//收发数据都是通过onRun线程
	//SendData只是将数据写入发送缓冲区
	//等待select检测可写时才会发送数据
	//旧的时间点
	auto t2 = Time::getNowInMilliSec();
	//新的时间点
	auto t0 = t2;
	//经过的时间
	auto dt = t0;
	Timestamp tTime;
	while (pThread->isRun())
	{
		t0 = Time::getNowInMilliSec();
		dt = t0 - t2;
		t2 = t0;
		//本次while (pThread->isRun())循环主要工作内容
		//代号work
		{
			int count = 0;
			//每轮每个客户端发送nMsg条数据
			for (int m = 0; m < nMsg; m++)
			{
				//每个客户端1条1条的写入消息
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
				{	//超时设置为0表示select检测状态后立即返回
					if (!clients[n]->OnRun(0))
					{	//连接断开
						nConnect--;
						continue;
					}
					//检测发送计数是否需要重置
					clients[n]->checkSend(dt);
				}
			}
		}
		Thread::Sleep(nWorkSleep);
	}
	//--------------------------
	//关闭消息收发线程
	//tRun.Close();
	//关闭客户端
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
	//设置运行日志名称
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

	//启动终端命令线程
	//用于接收运行时用户输入的指令
	Thread tCmd;
	tCmd.Start(nullptr, [](Thread* pThread) {
		while (true)
		{
			char cmdBuf[256] = {};
			scanf("%s", cmdBuf);
			if (0 == strcmp(cmdBuf, "exit"))
			{
				//pThread->Exit();
				CELLLog_Info("退出cmdThread线程");
				break;
			}
			else {
				CELLLog_Info("不支持的命令。");
			}
		}
	});

	//启动模拟客户端线程
	vector<Thread*> threads;
	for (int n = 0; n < nThread; n++)
	{
		Thread* t = new Thread();
		t->Start(nullptr, [n](Thread* pThread) {
			WorkThread(pThread, n + 1);
		});
		threads.push_back(t);
	}
	//每秒数据统计
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
	CELLLog_Info("。。已退出。。");
	return 0;
}
