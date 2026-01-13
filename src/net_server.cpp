#include "net_server.h"
#include "world.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>

NetServer::NetServer(int port) : listenPort(port) {}
NetServer::~NetServer(){ stop(); }

bool NetServer::start(World* world) {
    if (running.load()) return false;
    worldPtr = world;
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) { std::cerr<<"Server: socket failed\n"; return false; }
    int opt = 1; setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(listenPort);
    if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0) { std::cerr<<"Server: bind failed\n"; close(listenFd); return false; }
    if (listen(listenFd, 8) < 0) { std::cerr<<"Server: listen failed\n"; close(listenFd); return false; }
    running = true;
    acceptThread = std::thread(&NetServer::acceptLoop, this);
    std::cout << "Server: listening on port " << listenPort << "\n";
    return true;
}

void NetServer::stop(){
    running = false;
    if (listenFd>=0) { close(listenFd); listenFd = -1; }
    if (acceptThread.joinable()) acceptThread.join();
    std::lock_guard<std::mutex> lk(clientsMutex);
    for (int fd: clients) close(fd);
    clients.clear();
}

void NetServer::acceptLoop(){
    while (running) {
        sockaddr_in caddr; socklen_t len = sizeof(caddr);
        int cfd = accept(listenFd, (sockaddr*)&caddr, &len);
        if (cfd < 0) { if (running) std::cerr<<"Server: accept failed\n"; break; }
        {
            std::lock_guard<std::mutex> lk(clientsMutex);
            clients.push_back(cfd);
        }
        std::thread(&NetServer::clientLoop, this, cfd).detach();
        std::cout << "Server: client connected\n";
    }
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

void NetServer::clientLoop(int clientFd){
    std::string line;
    while (running && readlineFd(clientFd, line)) {
        // simple protocol: SET x y z type
        // or POS id x y z yaw pitch
        // echo SET to all clients and apply to server world
        if (line.rfind("SET ",0) == 0) {
            // parse
            int x,y,z,t;
            if (sscanf(line.c_str()+4, "%d %d %d %d", &x,&y,&z,&t) == 4) {
                worldPtr->setBlockAt(x,y,z, {static_cast<BlockType>(t)});
                broadcastLine(line + "\n");
            }
        } else if (line.rfind("POS ",0) == 0) {
            // for now just broadcast positions to others
            broadcastLine(line + "\n");
        }
    }
    // client disconnected
    close(clientFd);
    {
        std::lock_guard<std::mutex> lk(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientFd), clients.end());
    }
    std::cout << "Server: client disconnected\n";
}

void NetServer::broadcastLine(const std::string& line) {
    std::lock_guard<std::mutex> lk(clientsMutex);
    for (int fd : clients) {
        send(fd, line.c_str(), (int)line.size(), 0);
    }
}
