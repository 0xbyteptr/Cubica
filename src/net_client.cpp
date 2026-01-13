#include "net_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

NetClient* g_netClient = nullptr;

NetClient::NetClient(){}
NetClient::~NetClient(){ stop(); }
bool NetClient::start(const std::string& host, int port) {
    if (running.load()) return false;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { std::cerr<<"Client: socket failed\n"; return false; }
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) { std::cerr<<"Client: invalid host\n"; close(sock); return false; }
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { std::cerr<<"Client: connect failed\n"; close(sock); return false; }
    running = true;
    recvThread = std::thread(&NetClient::recvLoop, this);
    std::cout << "Client: connected to " << host << ":" << port << "\n";
    return true;
}

void NetClient::stop(){
    running = false;
    if (sock>=0) { close(sock); sock=-1; }
    if (recvThread.joinable()) recvThread.join();
}

bool NetClient::sendLine(const std::string& line){
    if (sock<0) return false;
    std::string l = line + "\n";
    ssize_t n = send(sock, l.c_str(), (int)l.size(), 0);
    return n == (ssize_t)l.size();
}

static bool readlineFd(int fd, std::string &out) {
    out.clear();
    char buf[256];
    while (true) {
        ssize_t n = recv(fd, buf, 1, 0);
        if (n <= 0) return false;
        if (buf[0] == '\n') break;
        out.push_back(buf[0]);
        if (out.size() > 4096) return false;
    }
    return true;
}

void NetClient::recvLoop(){
    std::string line;
    while (running && readlineFd(sock, line)) {
        // for now just print
        std::cout << "NetClient: recv: " << line << "\n";
    }
    std::cout << "Client: disconnected from server\n";
    running = false;
}
