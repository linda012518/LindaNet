#ifndef _HttpClientC_HPP_
#define _HttpClientC_HPP_

#include <map>
#include <string>
#include "Client.hpp"
#include "SplitString.hpp"
#include "KeyString.hpp"

namespace linda {
	namespace io {
		class HttpClientC : public Client
		{
		public:
			HttpClientC(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
				Client(sockfd, sendSize, recvSize)
			{

			}

			virtual bool hasMsg()
			{
				//完整的http响应一定超过20字节
				if (_recvBuff.dataLen() < 20)
					return false;
				int ret = checkHttpResponse();
				if (ret < 0)
				{
					CELLLog_Info("HttpClientC::checkHttpResponse error msg");
				}
				return ret > 0;
			}

			// -1 不支持响应类型
			// -2 异常响应
			int checkHttpResponse()
			{
				//查找http请求消息结束标记
				char* temp = strstr(_recvBuff.data(), "\r\n\r\n");
				if (!temp)//没找到
					return 0;
				//CELLLog_Info(_recvBuff.data());

				//指针计算消息头+响应行有多长    strlen("\r\n\r\n") = 4
				_headerLen = temp + 4 - _recvBuff.data();
				//判断响应类型
				temp = _recvBuff.data();
				if (temp[0] == 'H' && temp[1] == 'T' && temp[2] == 'T' && temp[3] == 'P')
				{
					//计算响应体长度
					char* p1 = strstr(_recvBuff.data(), "Content-Length: ");
					//未找到表示格式错误
					//返回错误或者直接关闭客户端连接
					//有长度的情况下，websocket有可能没有这一项
					if (p1)
					{
						//16 = strlen("Content-Length: ");
						p1 += 16;
						char* p2 = strchr(p1, '\r');
						if (!p2)
							return -2;
						//数字长度
						int n = p2 - p1;
						//6位数 99 9999 1MB
						//接收缓冲区一次性接收，最好分包
						if (n > 6)
							return -2;
						char lenStr[7] = {};
						strncpy(lenStr, p1, n);
						_bodyLen = atoi(lenStr);
						if (_bodyLen < 0)
							return -2;
						//post数据超过缓冲区大小
					}

					if (_headerLen + _bodyLen > _recvBuff.buffSize())
						return -2;
					//消息长度>已接收数据长度，数据还没接收完
					if (_headerLen + _bodyLen > _recvBuff.dataLen())
						return 0;
				}
				else {
					return -1;
				}

				return _headerLen;
			}

			//确定有消息，调用解析请求头
			bool getResponseInfo()
			{
				if (_headerLen <= 0)
					return false;

				//_recvBuff.data()[_headerLen] = '\0'
				char* pp = _recvBuff.data();
				pp[_headerLen - 1] = '\0';

				SplitString ss;
				ss.set(_recvBuff.data());
				//请求行
				char* temp = ss.get("\r\n");
				if (temp)
				{
					_header_map["ResponseLine"] = temp;
				}
				
				while (true)
				{
					temp = ss.get("\r\n");
					if (temp)
					{
						SplitString ss2;
						ss2.set(temp);

						char* key = ss2.get(": ");
						char* val = ss2.get(": ");
						if (key && val)
						{
							_header_map[key] = val;
						}
					}
					else
					{
						break;
					}
				}
				if (_bodyLen > 0)
				{
					//响应体
					//splitUrlArgs(_recvBuff.data() + _headerLen);
					_args_map["Content"] = _recvBuff.data() + _headerLen;
				}
				const char* str = header_getStr("Connection", "");
				//理论上还在再判断一下  "Upgrade: %s" 值是 websocket
				_keepalive = (0 == strcmp("Upgrade", str) || 0 == strcmp("keep-alive", str) || 0 == strcmp("Keep-Alive", str));
				//printf("%s", _keepalive);
				return true;
			}

			//解析响应内容
			//可以是html页面
			//只要可以解析http api返回的json文本字符串就可以
			void splitUrlArgs(char* args)
			{
				_args_map.clear();
				SplitString ss;
				ss.set(args);
				while (true)
				{
					char* temp = ss.get('&');
					if (temp)
					{
						SplitString ss2;
						ss2.set(temp);

						char* key = ss2.get('=');
						char* val = ss2.get('=');

						if (key && val)
						{
							_args_map[key] = val;
						}
					}
					else {
						break;
					}
				}
			}

			virtual void pop_front_msg()
			{
				if (_headerLen > 0)
				{
					_recvBuff.pop(_headerLen + _bodyLen);
					_headerLen = 0;
					_bodyLen = 0;

					_header_map.clear();
					_args_map.clear();
				}
			}

			bool has_args(const char* key)
			{
				return _args_map.find(key) != _args_map.end();
			}

			bool has_header(const char* key)
			{
				return _header_map.find(key) != _header_map.end();
			}

			int args_getInt(const char* argName, int def)
			{
				auto itr = _args_map.find(argName);
				if (itr == _args_map.end())
				{
					//日志
				}
				else
				{
					def = atoi(itr->second);
				}
				return def;
			}

			const char* args_getStr(const char* argName, const char* def)
			{
				auto itr = _args_map.find(argName);
				if (itr == _args_map.end())
				{
					//日志
				}
				else
				{
					def = itr->second;
				}
				return def;
			}

			const char* header_getStr(const char* argName, const char* def)
			{
				auto itr = _header_map.find(argName);
				if (itr != _header_map.end())
				{
					return itr->second;
				}
				else
				{
					return def;
				}
			}

			virtual void onRecvComplete()
			{
				if (!_keepalive)
				{
					this->onClose();
				}
			}

			const char* content()
			{
				return args_getStr("Content", nullptr);
			}
		protected:
			int _headerLen = 0;
			int _bodyLen = 0;
			std::map<KeyString, char*> _header_map;//std::string  不能调试
			std::map<KeyString, char*> _args_map;//std::string  不能调试
			bool _keepalive = true;//保持连接
		};

	}
}
#endif // !_CELLClientC_HPP_