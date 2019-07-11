#pragma once
#include <cpprest/http_listener.h>
#include <boost/asio/io_service.hpp>
#include <thread>
using namespace web;
using namespace http;
using namespace http::experimental::listener;

class HttpServer
{
public:
    HttpServer(const std::string &host, const int thr_num);

    std::string EndPoint();
    pplx::task<void> Accept();
    pplx::task<void> Shutdown();
    static std::vector<std::string> StringSplit(const std::string &s, const std::string &delim);

private:
    void OnRequest(http_request);
    void HandTest(http_request);
    void Base64Encode(const std::string & input, std::string &output);
    void Base64Decode(const std::string &input, std::string &output);

    http_listener listener_;
    std::mutex hander_mtx_;
    std::map<std::string, std::function<void(http_request)>> handler_map_;

    boost::asio::io_service io_service_;
    boost::asio::io_service::work io_work_;
    std::vector<std::thread> threads_vec_;
};
