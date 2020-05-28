#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include "client.h"

void mainClient();
void mainServer();

int main(int argc, char* argv[]) {
	if (strcmp(argv[1], "-s")==0) {
		std::cout << "\nSERVER\n";
		mainServer();
	}
	else if (strcmp(argv[1], "-c") == 0) {
		std::cout << "\nCLIENT\n";
		mainClient();
	}
	return 0;
}


void mainClient() {
	WSADATA wsaData{0};
	const int res = WSAStartup(MAKEWORD(2, 2), &wsaData);

	basic_net::ActiveSocket sock{"14009", "127.0.0.1"};
	std::vector<char> sendBuff {'0', 'H', 'e', 'l', 'l', 'o', '\0'};
	std::vector<char> recvBuff (2048);
	sock.send(sendBuff);
	Sleep(3000);
	sock.receive(recvBuff);
	std::cout << "\nReceived message: " << recvBuff.data() << '\n';
}


void answer(basic_net::ActiveSocket as) {
	std::vector<char> recvBuff(2048);

	as.receive(recvBuff);
	std::vector<char> sendBuff;
	sendBuff.emplace_back(recvBuff[0]);
	sendBuff.emplace_back('H');
	sendBuff.emplace_back('i');
	sendBuff.emplace_back('\0');

	as.send(sendBuff);
	Sleep(10000);
}


void mainServer() {
	WSADATA wsaData{0};
	const int res = WSAStartup(MAKEWORD(2, 2), &wsaData);

	basic_net::ListeningSocket sock{"14009"};
	basic_net::Listener listen{sock};
	/*std::thread close{ [&sock](){Sleep(3000); 
								 sock.close();}};*/
	listen.listen(answer);
}
