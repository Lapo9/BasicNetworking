#ifndef BASICNETWORKING_COMPLENTIONPORT
#define BASICNETWORKING_COMPLENTIONPORT

#include <WinSock2.h>
#include <WS2tcpip.h>

namespace basic_net {

class IOCP {
	
	HANDLE iocp;

	public:
	IOCP(unsigned long threadsNumber = 0) {
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, threadsNumber);
		if(iocp == NULL) {/*TODO throw*/ }
	}

	~IOCP() noexcept {
		const int err = CloseHandle(this);
		if(!err) {/*TODO throw error (so terminate the program since it is noexcept)*/ }
	}


	//THINK HANDLE type must always be a OverlappedIO, so maybe I can enforce this (maybe templatize this class)
	void associate(HANDLE newHandle, unsigned long key) {
		const HANDLE err = CreateIoCompletionPort(newHandle, iocp, key, 0);
		if(err != iocp) {/*TODO throw*/ }
	}

};

}

#endif

