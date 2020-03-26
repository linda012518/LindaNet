#ifndef _HttpClientS_HPP_
#define _HttpClientS_HPP_

#include <map>
#include <string>
#include "Client.hpp"
#include "SplitString.hpp"
#include "KeyString.hpp"

namespace linda {
	namespace io {
		class HttpClientS : public Client
		{
		public:
			enum RequestType
			{
				GET = 10,
				POST,
				UNKNOW
			};
		public:
			HttpClientS(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
				Client(sockfd, sendSize, recvSize)
			{

			}

			virtual bool hasMsg()
			{
				//完整的http请求一定超过20字节
				if (_recvBuff.dataLen() < 20)
					return false;
				int ret = checkHttpRequest();
				if (ret < 0)
				{
					response400BadRequest();
				}
				return ret > 0;
			}

			// -1 不支持请求类型
			// -2 异常请求
			int checkHttpRequest()
			{
				//查找http请求消息结束标记
				char* temp = strstr(_recvBuff.data(), "\r\n\r\n");
				if (!temp)//没找到
					return 0;
				//CELLLog_Info(_recvBuff.data());

				//指针计算消息头+请求行有多长    strlen("\r\n\r\n") = 4
				_headerLen = temp + 4 - _recvBuff.data();
				//判断请求类型
				temp = _recvBuff.data();
				if (temp[0] == 'G' && temp[1] == 'E' && temp[2] == 'T')
				{
					_requestType = HttpClientS::GET;
				}
				else if (temp[0] == 'P' && temp[1] == 'O' && temp[2] == 'S' && temp[3] == 'T')
				{
					_requestType = HttpClientS::POST;
					//post需要计算请求体长度
					char* p1 = strstr(_recvBuff.data(), "Content-Length: ");
					//未找到表示格式错误
					//返回错误或者直接关闭客户端连接
					if (!p1)
						return -2;
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
					if (_headerLen + _bodyLen > _recvBuff.buffSize())
						return -2;
					//消息长度>已接收数据长度，数据还没接收完
					if (_headerLen + _bodyLen > _recvBuff.dataLen())
						return 0;
				}
				else
				{
					_requestType = HttpClientS::UNKNOW;
					return -1;
				}

				return _headerLen;
			}

			//确定有消息，调用解析请求头
			bool getRequestInfo()
			{
				if (_headerLen <= 0)
					return false;

				SplitString ss;
				ss.set(_recvBuff.data());
				//请求行
				char* temp = ss.get("\r\n");
				if (temp)
				{
					if (_requestType == HttpClientS::GET || _requestType == HttpClientS::POST)
						_header_map["RequestLine"] = temp;
					else
						return false;

					request_args(temp);
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
					//请求体
					splitUrlArgs(_recvBuff.data() + _headerLen);
				}
				const char* str = header_getStr("Connection", "");
				_keepalive = (
					   0 == strcmp("keep-alive", str)
					|| 0 == strcmp("Keep-Alive", str)
					|| 0 == strcmp("Upgrade", str)
					|| 0 == strcmp("keep-alive, Upgrade", str)
					);
				//printf("%s", _keepalive);
				//linda!!!不保持长连接，就要下边这句话
				//_keepalive = false;
				return true;
			}

			bool request_args(char* requestLine)
			{
				SplitString ss;
				ss.set(requestLine);
				_method = ss.get(' ');
				if (!_method)
					return false;

				_url = ss.get(' ');
				if (!_url)
					return false;

				_httpVersion = ss.get(' ');
				if (!_httpVersion)
					return false;

				ss.set(_url);
				_url_path = ss.get("?");
				if (!_url_path)
					return false;

				_url_args = ss.get("?");
				if (!_url_args)//参数可以没有
					return true;

				splitUrlArgs(_url_args);
				return true;
			}

			void splitUrlArgs(char* args)
			{
				//_args_map.clear();
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
					_header_map.clear();//解析完就清空
					_args_map.clear();//解析完就清空参数  收完解析完
				}
			}

			bool canWrite(int size)
			{
				return _sendBuff.canWrite(size);
			}

			void response400BadRequest()
			{
				writeResponse("400 Bad Request", "Only suppotr GET or POST", 25);
			}

			void response404NotFound()
			{
				writeResponse("404 Not Found", "404--???????", 11);
			}

			void response200OK(const char* bodyBuff, int bodyLen)
			{
				writeResponse("200 OK", bodyBuff, bodyLen);
			}

			void writeResponse(const char* code, const char* bodyBuff, int bodyLen)
			{
				//响应行+响应头缓冲区
				char response[256] = {};
				//响应行
				strcat(response, "HTTP/1.1 ");
				strcat(response, code);
				strcat(response, "\r\n");
				//响应头
				strcat(response, "Content-Type: text/html;charset=UTF-8\r\n");
				strcat(response, "Access-Control-Allow-Origin: *\r\n");//跨域要这样
				strcat(response, "Connection: Keep-Alive\r\n");//跨域要这样
				//linda!!!不保持长连接，就要下边这句话，上边那句不要
				//strcat(response, "Connection: close\r\n");//跨域要这样
				//响应体长度
				char respBodyLen[32] = {};
				sprintf(respBodyLen, "Content-Length: %d\r\n", bodyLen);

				strcat(response, respBodyLen);
				strcat(response, "\r\n");
				//发送响应体
				SendData(response, strlen(response));
				SendData(bodyBuff, bodyLen);
			}

			char* url()
			{
				return _url_path;
			}

			bool url_compare(const char* str)
			{
				return 0 == strcmp(_url_path, str);
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

			virtual void onSendComplete()
			{
				if (!_keepalive)
				{
					this->onClose();
				}
			}

		protected:
			int _headerLen = 0;
			int _bodyLen = 0;
			std::map<KeyString, char*> _header_map;//std::string  不能调试
			std::map<KeyString, char*> _args_map;//std::string  不能调试
			RequestType _requestType = HttpClientS::UNKNOW;
			char* _method;
			char* _url;
			char* _url_path;
			char* _url_args;
			char* _httpVersion;
			bool _keepalive = true;//保持连接
		};

	}
}
#endif // !_CELLClientS_HPP_