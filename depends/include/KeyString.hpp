#ifndef _KeyString_HPP_
#define _KeyString_HPP_

#include <cstring>

namespace linda {
	namespace io {
		//网络消息接收处理服务类
		class KeyString
		{
		private:
			const char* _str = nullptr;
		public:
			KeyString(const char* str)
			{
				set(str);
			}

			void set(const char* str)
			{
				_str = str;
			}

			const char* get()
			{
				return _str;
			}

			friend bool operator < (const KeyString& left, const KeyString& right);

			//bool operator < (const KeyString& right)
			//{
			//	return strcmp(this->_str, right._str) < 0;
			//}
		};
		//不要加static vc++和ubuntu g++ 编译不一样，也要用友员函数
		bool operator < (const KeyString& left, const KeyString& right)
		{
			return strcmp(left._str, right._str) < 0;
		}

		//bool operator < (const KeyString& left, const KeyString& right)
		//{
		//	return left < right;
		//}
	}
}
#endif // !_KeyString_HPP_
