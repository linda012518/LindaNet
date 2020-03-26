#include "Log.hpp"
#include "Config.hpp"
#include "TcpHttpServer.hpp"

using namespace linda::io;

class MyServer : public TcpHttpServer
{
	virtual void OnNetMsgHttp(Server* pServer, HttpClientS* httpClient)
	{

		if (httpClient->url_compare("/add"))
		{
			int a = httpClient->args_getInt("a", 0);
			int b = httpClient->args_getInt("b", 0);
			int c = a + b;

			char responseBody[32] = {};
			sprintf(responseBody, "a + b = %d", c);

			httpClient->response200OK(responseBody, strlen(responseBody));
		}
		else
		{
			httpClient->response404NotFound();
		}
	}
};

int main(int argc, char* args[])
{
	//设置运行日志名称
	Log::Instance().setLogPath("serverLog", "w", false);
	Config::Instance().Init(argc, args);

	const char* strIP = Config::Instance().getStr("strIP", "any");
	uint16_t nPort = Config::Instance().getInt("nPort", 4567);
	int nThread = Config::Instance().getInt("nThread", 1);

	if (strcmp(strIP, "any") == 0)
	{
		strIP = nullptr;
	}

	MyServer server;
	if (Config::Instance().hasKey("-ipv6"))
	{
		CELLLog_Info("-ipv6");
		server.InitSocket(AF_INET6);
	}
	else
	{
		CELLLog_Info("-ipv4");
		server.InitSocket();
	}

	server.Bind(strIP, nPort);
	server.Listen(SOMAXCONN);
	server.Start(nThread);


	//在主线程中等待用户输入命令
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