#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include "spdlog/spdlog.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/PartHandler.h"
#include "Poco/StreamCopier.h"
#include "http_server.h"

namespace Poco{
    namespace Net{
        class StringPartHandler: public PartHandler
        {
        public:
            StringPartHandler()
            {
            }

            void handlePart(const MessageHeader& header, std::istream& stream)
            {
                std::string disp = header["Content-Disposition"];
                disp = disp.substr(disp.find_first_of(';') + 1);
                std::vector<std::string> infoVec = HttpServer::StringSplit(disp, ";");
                std::map<std::string,std::string> disp_map;
                for(std::string &disp_str : infoVec)
                {
                    int pos = disp_str.find_first_of('=');
                    std::string key = disp_str.substr(0, pos);
                    key.erase(std::remove(key.begin(),key.end(), ' '), key.end());
                    key.erase(std::remove(key.begin(),key.end(), '"'), key.end());

                    std::string value = disp_str.substr(pos + 1);
                    value.erase(std::remove(value.begin(),value.end(), ' '), value.end());
                    value.erase(std::remove(value.begin(),value.end(), '"'), value.end());
                    disp_map[key] = value;
                }
                auto ilter = disp_map.find("name");
                if(ilter == disp_map.end()){
                    return;
                }
                std::string name = ilter->second;

                ilter = disp_map.find("filename");
                if(ilter == disp_map.end()){
                    return;
                }
                std::string filename = ilter->second;

                std::ostringstream  ostr;
                Poco::StreamCopier::copyStream(stream, ostr);
                part_map_.insert(std::make_pair(name, std::make_pair(filename, ostr.str())));
            }

            bool HasFiled(const std::string &name){
                return part_map_.find(name) != part_map_.end();
            }

            const std::pair<std::string, std::string>& FileData(const std::string &name) const{
                auto ilter = part_map_.find(name);
                if(ilter != part_map_.end()){
                    return ilter->second;
                }

                return std::pair<std::string, std::string>();
            }

            const std::map<std::string, std::pair<std::string, std::string>>& PartMap() const{
                return part_map_;
            }

        private:
            std::map<std::string, std::pair<std::string, std::string>> part_map_;
        };
    }
}

HttpServer::HttpServer(const std::string &host, const int thr_num):
    io_service_(thr_num),io_work_(io_service_)
{
    for(int i = 0; i < thr_num; i++){
        threads_vec_.emplace_back(std::thread([this]{
            io_service_.run();
        }));
    }
    listener_ = http_listener(host);
    listener_.support(std::bind(&HttpServer::OnRequest, this, std::placeholders::_1));
    handler_map_.insert(std::make_pair("/rest/api/v1/test", std::bind(&HttpServer::HandTest,this, std::placeholders::_1)));
}

std::string HttpServer::EndPoint()
{
    return listener_.uri().to_string();
}

pplx::task<void> HttpServer::Accept()
{
    return listener_.open();
}

pplx::task<void> HttpServer::Shutdown()
{
    return listener_.close();
}

std::vector<std::string> HttpServer::StringSplit(const std::string &s, const std::string &delim)
{
    std::vector<std::string> ret;
    size_t last = 0;
//    std::string_view sv(s);
    size_t index = s.find_first_of(delim, last);
    while(index != std::string::npos){//查找到匹配
        ret.push_back(std::string{s.substr(last, index-last)});
        last = index + 1;
        index = s.find_first_of(delim, last);
    }
    if(index - last > 0)ret.push_back(std::string{s.substr(last, index-last)});

    return ret;
}

void HttpServer::OnRequest(http_request message)
{
    auto response = json::value::object();
    std::string url_path = message.relative_uri().path();
    spdlog::info("Received request, path: {}", url_path);

    if(message.method() == methods::POST){
        io_service_.post([=]{
            try{
                std::function<void(http_request)> handler;
                bool is_valid_path = false;
                {
                    std::lock_guard<std::mutex> lock(hander_mtx_);
                    auto iter = handler_map_.find(url_path);
                    if (iter != handler_map_.end()) {
                        handler = iter->second;
                        is_valid_path = true;
                    }
                }

                if (is_valid_path) {
                    handler(message);
                }else{
                    response["status"] = json::value::number(20001);

                    std::string msg("request an unknown uri: ");
                    msg += url_path;

                    response["message"] = json::value::string(msg);
                    message.reply(status_codes::BadRequest, response);

                    spdlog::trace("unknown path: {}", url_path);
                }
            }catch(std::exception &e){
                response["status"] = json::value::number(20001);
                response["message"] = json::value::string(e.what());
                message.reply(status_codes::InternalError, response);

                spdlog::error("{}:{}",__FUNCTION__, e.what());
            }catch(...){
                response["status"] = json::value::number(20001);
                response["message"] = json::value::string("unknown error");
                message.reply(status_codes::InternalError, response);

                spdlog::error("{}: unknown error", __func__);
            }
        });
    }else{
        response["status"] = json::value::number(20001);
        response["message"] = json::value::string("unsupported methord");
        message.reply(status_codes::NotImplemented, response);

        spdlog::warn("NotImplemented methord called");
    }
}

void HttpServer::HandTest(http_request message)
{

}

bool HttpServer::Base64Encode(const std::string &input, std::string &output)
{
    typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8>> Base64EncodeIterator;
    std::stringstream result;
    try {
        std::copy( Base64EncodeIterator( input.begin() ), Base64EncodeIterator( input.end() ), std::ostream_iterator<char>( result ) );
    } catch ( ... ) {
        return false;
    }
    size_t equal_count = (3 - input.length() % 3) % 3;
    for ( size_t i = 0; i < equal_count; i++ )
    {
        result.put( '=' );
    }
    output = result.str();
    return output.empty() == false;
}

bool HttpServer::Base64Decode(const std::string &input, std::string &output)
{
    typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6> Base64DecodeIterator;
    std::stringstream result;
    try {
        std::copy( Base64DecodeIterator( input.begin() ), Base64DecodeIterator( input.end() ), std::ostream_iterator<char>( result ) );
    } catch ( ... ) {
        return false;
    }
    output = result.str();
    return output.empty() == false;
}
