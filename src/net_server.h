#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <vector>

class World;

class NetServer {
public:
    NetServer(int port = 25565);
    ~NetServer();
    bool start(World* world);
    void stop();
private:
    int listenPort;
    int listenFd = -1;
    std::thread acceptThread;
    std::atomic<bool> running{false};
    World* worldPtr = nullptr;
    std::vector<int> clients;
    std::mutex clientsMutex;

    void acceptLoop();
    void clientLoop(int clientFd);
    void broadcastLine(const std::string& line);
};