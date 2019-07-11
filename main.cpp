#include <iostream>
#include "spdlog/spdlog.h"
#include "http_server.h"

std::mutex mtx;
std::condition_variable cv_;
void handleUserInterrupt(int signal){
    if (signal == SIGINT) {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "SIGINT trapped ..." << std::endl;
        cv_.notify_one();
    }
}

int main()
{
    try{
        HttpServer server("http://0.0.0.0:6605", 30);
        server.Accept().wait();
        spdlog::info("Microsvc is listening request on {}", server.EndPoint());

        signal(SIGINT, handleUserInterrupt);
        std::unique_lock<std::mutex> lock(mtx);
        cv_.wait(lock);

        server.Shutdown().wait();
    }catch(std::exception &e){
        spdlog::critical("StartServer exception: {}", e.what());
    }
    return 0;
}
