#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_

#include"CELL.hpp"
//#include"Task.hpp"
#include<ctime>

namespace linda {
	namespace io {
		class Log
		{
			//Info  普通信息
			//Debug 调试信息，只在debug模式起作用
			//Warring 警告信息
			//Error 错误信息
#ifdef _DEBUG
#ifndef CELLLog_Debug
#define CELLLog_Debug(...) Log::Debug(__VA_ARGS__)
#endif
#else
#ifndef CELLLog_Debug
#define CELLLog_Debug(...)
#endif
#endif // _DEBUG

#define CELLLog_Info(...) Log::Info(__VA_ARGS__)
#define CELLLog_Warring(...) Log::Warring(__VA_ARGS__)
#define CELLLog_Error(...) Log::Error(__VA_ARGS__)
#define CELLLog_Error(...) Log::Error(__VA_ARGS__)
#define CELLLog_PError(...) Log::PError(__VA_ARGS__)

		private:
			Log()
			{
				_taskServer.Start();
			}

			~Log()
			{
				_taskServer.Close();
				if (_logFile)
				{
					Info("Log fclose(_logFile)");
					fclose(_logFile);
					_logFile = nullptr;
				}
			}
		public:
			static Log& Instance()
			{
				static  Log sLog;
				return sLog;
			}

			void setLogPath(const char* logName, const char* mode, bool hasDate)
			{
				if (_logFile)
				{
					Info("Log::setLogPath _logFile != nullptr");
					fclose(_logFile);
					_logFile = nullptr;
				}
				//
				char logPath[256] = {};
				//
				if (hasDate)
				{
					auto t = std::chrono::system_clock::now();
					auto tNow = std::chrono::system_clock::to_time_t(t);
					std::tm* now = std::localtime(&tNow);
					sprintf(logPath, "%s[%d-%d-%d_%d-%d-%d].txt", logName, now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				}
				else {
					sprintf(logPath, "%s.txt", logName);
				}

				//
				_logFile = fopen(logPath, mode);
				if (_logFile)
				{
					Info("Log::setLogPath success,<%s,%s>", logPath, mode);
				}
				else {
					Info("Log::setLogPath failed,<%s,%s>", logPath, mode);
				}
			}

			static void PError(const char* pStr)
			{
				PError("%s", pStr);
			}

			template<typename ...Args>
			static void PError(const char* pformat, Args ... args)
			{
				char* logStr = newFormatStr(pformat, args...);
#ifdef _WIN32
				auto errCode = GetLastError();

				Instance()._taskServer.addTask([errCode, logStr]() {

					char text[256] = {};
					FormatMessageA(
						FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						errCode,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPSTR)&text, 256, NULL
					);

					EchoReal(true, "PError ", "%s", logStr);
					EchoReal(false, "PError ", "errno=%d,errmsg=%s", errCode, text);
					delete[] logStr;
				});
#else
				auto errCode = errno;
				Instance()._taskServer.addTask([errCode, logStr]() {
					EchoReal(true, "PError ", "%s", logStr);
					EchoReal(true, "PError ", "errno<%d>,errmsg<%s>", errCode, strerror(errCode));
					delete[] logStr;
				});
#endif
			}

			static void Error(const char* pStr)
			{
				Error("%s", pStr);
			}

			template<typename ...Args>
			static void Error(const char* pformat, Args ... args)
			{
				Echo("Error ", pformat, args...);
			}

			static void Warring(const char* pStr)
			{
				Warring("%s", pStr);
			}

			template<typename ...Args>
			static void Warring(const char* pformat, Args ... args)
			{
				Echo("Warring ", pformat, args...);
			}

			static void Debug(const char* pStr)
			{
				Debug("%s", pStr);
			}

			template<typename ...Args>
			static void Debug(const char* pformat, Args ... args)
			{
				Echo("Debug ", pformat, args...);
			}

			static void Info(const char* pStr)
			{
				Info("%s", pStr);
			}

			template<typename ...Args>
			static void Info(const char* pformat, Args ... args)
			{
				Echo("Info ", pformat, args...);
			}

			template<typename ...Args>
			static void Echo(const char* type, const char* pformat, Args ... args)
			{
				char* logStr = newFormatStr(pformat, args...);
				Log* pLog = &Instance();
				pLog->_taskServer.addTask([type, logStr]() {
					EchoReal(true, type, "%s", logStr);
					delete[] logStr;
				});
			}

			template<typename ...Args>
			static void EchoReal(bool br, const char* type, const char* pformat, Args ... args)
			{
				Log* pLog = &Instance();
				if (pLog->_logFile)
				{
					auto t = std::chrono::system_clock::now();
					auto tNow = std::chrono::system_clock::to_time_t(t);
					//fprintf(pLog->_logFile, "%s", ctime(&tNow));
					std::tm* now = std::localtime(&tNow);
					if (type)
						fprintf(pLog->_logFile, "%s", type);
					fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
					fprintf(pLog->_logFile, pformat, args...);
					if (br)
						fprintf(pLog->_logFile, "%s", "\n");
					fflush(pLog->_logFile);

				}
				if (type)
					printf("%s", type);
				printf(pformat, args...);
				if (br)
					printf("%s", "\n");
			}

			template<typename ...Args>
			static char* newFormatStr(const char* pformat, Args ... args)
			{
				//计算格式化字符串所需要的char字符数量
				//+1 是为\0 结束符
				size_t n = 1 + snprintf(nullptr, 0, pformat, args...);
				char* pFormatStr = new char[n];
				snprintf(pFormatStr, n, pformat, args...);
				return pFormatStr;
			}

		private:
			FILE* _logFile = nullptr;
			TaskServer _taskServer;
		};
	}
}
#endif // !_CELL_LOG_HPP_
