#ifndef _TcpClientMgr_hpp_
#define _TcpClientMgr_hpp_

#ifdef __linux__
	#include "TcpEpollClient.hpp"
#else
	#include "TcpSelectClient.hpp"
#endif // __linux__



namespace linda {
	namespace io {

#ifdef __linux__
		typedef TcpEpollClient TcpClientMgr;
#else
		typedef TcpSelectClient TcpClientMgr;
#endif // __linux__
	}
}
#endif // !_TcpClientMgr_hpp_
