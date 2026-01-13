#pragma once
#include <string>
#include <thread>
#include <atomic>

class NetClient {
public:
    NetClient(); ~NetClient();
    bool start(const std::string& host, int port);
    void stop();
    bool sendLine(const std::string& line);
private:
    int sock = -1;
    std::thread recvThread;
    std::atomic<bool> running{false};
    void recvLoop();
};

// global pointer (set in main when starting client)
extern NetClient* g_netClient;