#ifndef _TcpServerMgr_hpp_
#define _TcpServerMgr_hpp_

#ifdef __linux__
	#include "TcpEpollServer.hpp"
#else
	#include "TcpSelectServer.hpp"
#endif // __linux__



namespace linda {
	namespace io {

#ifdef __linux__
		typedef TcpEpollServer TcpServerMgr;
#else
		typedef TcpSelectServer TcpServerMgr;
#endif // __linux__
	}
}
#endif // !_TcpServerMgr_hpp_
