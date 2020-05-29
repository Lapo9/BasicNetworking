#ifndef BASICNETWORKING_CLIENT
#define BASICNETWORKING_CLIENT

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include "notifier.h"

namespace basic_net {

using n = notifier::Notifier;


class SocketError : public std::exception {
	int code;
	int error;

public:
	SocketError(int code) : code{code}, error{::WSAGetLastError()} {
		std::cout << "\n==== ERROR: " << error << " ====\n";
	}

	int getError() {
		return error;
	}

};


class Addrinfo {
	
	::addrinfo* addrinfo;

	public:
	Addrinfo(const std::string& port, const std::string& serverName, int family =AF_UNSPEC, int socktype =SOCK_STREAM, int protocol =IPPROTO_TCP, int flags =0) {
		::addrinfo hints = socketSpecs(family, socktype, protocol); //build the specs for this socket
		
		const int err = ::getaddrinfo(serverName.c_str(), port.c_str(), &hints, &addrinfo); //get IP address of serverName through DNS
		if (err) {throw SocketError{err};}
	}

	Addrinfo() : addrinfo{nullptr} {}

	Addrinfo(const Addrinfo&) = default;
	Addrinfo(Addrinfo&&) = default;
	Addrinfo& operator=(const Addrinfo&) = default;
	Addrinfo& operator=(Addrinfo&&) = default;

	~Addrinfo() {
		::freeaddrinfo(addrinfo);
	}

	const ::addrinfo* get() const {
		return addrinfo;
	}


	private:
	static ::addrinfo socketSpecs(int family =AF_UNSPEC, int socktype =SOCK_STREAM, int protocol =IPPROTO_TCP, int flags =0) {
		::addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = socktype;
		hints.ai_protocol = protocol;
		hints.ai_flags = flags;

		return hints;
	}
};



class Socket {
	
	protected:
	::SOCKET socket = INVALID_SOCKET;
	const Addrinfo addrinfo;

	~Socket() noexcept {
		n::notify("destroying socket...");
		//TODO maybe check if ::closesocket(socket) has been called?
	}


	public:
	/**
	 * @brief Creates a new Socket object with the specified parameters.
	 * @details By default a TCP socket is created
	 * @param port The port of the socket
	 * @param serverName Either the IP or the name of the server to which connect. If not specified (such as for a listening socket) it set to NULL.
	 * @param family The family of the socket
	 * @param socktype The type of the socket
	 * @param protocol The protocol used by the socket
	 * @param flags Optional flags
	*/
	Socket(const std::string& port, const std::string& serverName = NULL, int family = AF_UNSPEC, int socktype = SOCK_STREAM, int protocol = IPPROTO_TCP, int flags =0)
		: addrinfo{port, serverName, family, socktype, protocol, flags} {
		n::notify("creating socket...");
		socket = ::socket(addrinfo.get()->ai_family, addrinfo.get()->ai_socktype, addrinfo.get()->ai_protocol);
		if (socket == INVALID_SOCKET) {throw SocketError{(int)INVALID_SOCKET};}
	}

	//THINK these ctors (and = operators) what should do? Should the copy be a deep copy (basically establishing a new connection with a different SOCKET descriptor)?
	/**
	 * @brief Creates a Socket object given a socket descriptor.
	 * @details The new socket uses the same descriptor that is passed in. This means that if the SOCKET descriptor passed in is closed (via closesocket(socket)), all of the sockets using that descriptor cannot be used anymore.
	 * @param plainSocket 
	 * @return 
	*/
	Socket(::SOCKET plainSocket) {
		n::notify("creating socket...");
		socket = plainSocket;
	}

	Socket(const Socket&) = default;
	Socket(Socket&&) = default;
	Socket& operator= (const Socket&) = default;
	Socket& operator= (Socket&&) = default;


	//TODO is shutdown needed?
	/**
	 * @brief Closes the connection and call the destructor.
	*/
	void close() {
		n::notify("closing socket...");
		::closesocket(socket);
		this->~Socket();
	}

};


//THINK not a gread definition of buffer, but for now it does the work
template <typename T>
concept Buffer = requires (T buffer) {
	(char*)buffer.data();
	(int)buffer.capacity();
};

class ActiveSocket final : public Socket {
	
	public:
	ActiveSocket(const std::string& port, const std::string& serverName, int family = AF_UNSPEC, int socktype = SOCK_STREAM, int protocol = IPPROTO_TCP)
		: Socket{port, serverName, family, socktype, protocol} {
		connect();
	}

	ActiveSocket(SOCKET plainSocket) 
		: Socket{plainSocket} {
		//connect();
	}
	
	ActiveSocket(const ActiveSocket&) = default;
	ActiveSocket(ActiveSocket&&) = default;
	ActiveSocket& operator= (const ActiveSocket&) = default;
	ActiveSocket& operator= (ActiveSocket&&) = default;

	~ActiveSocket() = default;

	/**
	 * @brief Connects the socket to the address-port tuple specified at construction.
	*/
	void connect() {
		n::notify("connecting socket...");
		const int err = ::connect(socket, addrinfo.get()->ai_addr, addrinfo.get()->ai_addrlen);
		if (err) {
			throw SocketError{err};
		}
	}

	/**
	 * @brief Reads the buffer
	 * @param buffer 
	 * @return 
	*/
	template <Buffer Buff = std::vector<char>>
	int receive (Buff& buffer) {
		n::notify("receiving...");
		const int received = ::recv(socket, buffer.data(), buffer.capacity(), 0);
		if (received == SOCKET_ERROR) {throw SocketError{SOCKET_ERROR};}
		n::notify(std::to_string(received) + " bits received");
		return received;
	}

	template <Buffer Buff = std::vector<char>>
	int send(Buff& buffer) {
		n::notify("sending...");
		const int sent = ::send(socket, buffer.data(), buffer.capacity(), 0);
		if (sent == SOCKET_ERROR) {throw SocketError{SOCKET_ERROR};}
		n::notify(std::to_string(sent) + " bits sent");
		return sent;
	}

};



class ListeningSocket final : public Socket {

	public:
	ListeningSocket(const std::string& port,int family = AF_UNSPEC, int socktype = SOCK_STREAM, int protocol = IPPROTO_TCP)
		: Socket{port, "192.168.1.98", family, socktype, protocol, AI_PASSIVE} {
		bind();
	}
	
	ListeningSocket(const ListeningSocket&) = default;
	ListeningSocket(ListeningSocket&&) = default;
	ListeningSocket& operator= (const ListeningSocket&) = default;
	ListeningSocket& operator= (ListeningSocket&&) = default;

	~ListeningSocket() = default;


	ActiveSocket accept() {
		n::notify("accepting new connection...");
		SOCKET plainSocket = ::accept(socket, NULL, NULL);
		if (plainSocket == INVALID_SOCKET) {throw SocketError{(int)INVALID_SOCKET};}
		return ActiveSocket(plainSocket);
	}

	void listen() {
		n::notify("listening for new connections...");
		const int err = ::listen(socket, SOMAXCONN);
		if (err == SOCKET_ERROR) {throw SocketError{SOCKET_ERROR};}
	}

	void bind() {
		n::notify("binding socket...");
		const int err = ::bind(socket, addrinfo.get()->ai_addr, addrinfo.get()->ai_addrlen);
		if (err == SOCKET_ERROR) {throw SocketError{SOCKET_ERROR};}
	}

};



class Listener {
	
	ListeningSocket* socket;

	public:
	Listener(ListeningSocket& socket /*params to choose how to listen and how to manage accepted connections*/) 
		: socket{&socket} {
		
	}

	/**
	 * @brief Starts listening for new connection on the member ListeningSocket. 
	 * @details When a new connection is detected it is accepted, a new ActiveSocket to deal with such connection is created and passed to a new thread.
	 * @param manageAcceptedConnection The function that has to deal with the new connection. The new ActiveSocket created when the connection is accepted is passed ad argument. This function execute on a new thread.
	*/
	void listen(std::function<void(ActiveSocket)> manageAcceptedConnection) {
		socket->listen();
		while (true) {
			std::thread t{manageAcceptedConnection, socket->accept()};
			n::notify("thread for new socket created");
			t.detach();
		}
	}


};

}

#endif
