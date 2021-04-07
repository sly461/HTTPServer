/**
 * 利用有限状态自动机解析http请求
 */
#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <regex>
#include <unordered_map>

#include "../buffer/buffer.h"

class HTTPRequest {
public:
    //请求行、请求头、请求体
    enum PARSE_STATE {
        REQUEST_LINE,
        REQUEST_HEADER,
        REQUEST_BODY,
        FINISH
    };
    HTTPRequest();
    ~HTTPRequest() = default;

    void Init();

    std::string GetMethod() const;
    std::string GetPath() const;
    std::string GetVersion() const;

    bool IsKeepAlive() const;

    bool Parse(Buffer& buffer);

private:
    PARSE_STATE m_state;

    //方法 路径 版本
    std::string m_method;
    std::string m_path;
    std::string m_version;
    
    //请求头
    std::unordered_map<std::string, std::string> m_header;
};

#endif