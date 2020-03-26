#ifndef _SplitString_HPP_
#define _SplitString_HPP_

#include <string>

namespace linda {
	namespace io {
		//网络消息接收处理服务类
		class SplitString
		{
		private:
			char* _str = nullptr;
			bool _first = true;
		public:
			void set(char* str)
			{
				_str = str;
				_first = true;
			}

			char* get(char end)
			{
				if (!_str)
					return nullptr;

				char* temp = strchr(_str, end);//strchr找一个字符 strstr找多个
				if (!temp)
				{
					if (!_first)
						return nullptr;
					_first = false;
					return _str;
				}
				//第一个\r\n  把 \r 改成 \0 结束符
				temp[0] = '\0';
				char* ret = _str;
				_str = temp + 1;
				return ret;
			}

			char* get(const char* end)
			{
				if (!_str || !end)
					return nullptr;

				char* temp = strstr(_str, end);//strchr找一个字符 strstr找多个
				if (!temp)
				{
					if (!_first)
						return nullptr;
					_first = false;
					return _str;
				}
				//第一个\r\n  把 \r 改成 \0 结束符
				temp[0] = '\0';
				char* ret = _str;
				_str = temp + strlen(end);
				return ret;
			}

		};
	}
}
#endif // !_SplitString_HPP_
