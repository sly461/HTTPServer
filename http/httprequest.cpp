#include "httprequest.h"



HTTPRequest::HTTPRequest() {
    Init();
}

void HTTPRequest::Init() {
    m_method = m_path = m_version = "";
    m_state = REQUEST_LINE;
    m_header.clear();
}

std::string HTTPRequest::GetMethod() const {
    return m_method;
}
    
std::string HTTPRequest::GetPath() const {
    return m_path;
}
    
std::string HTTPRequest::GetVersion() const {
    return m_version;
}

bool HTTPRequest::IsKeepAlive() const {
    if(!m_header.count("Connection")) return false;
    //此处不能用[] 因为是const函数
    return m_header.find("Connection")->second == "keep-alive";
}

bool HTTPRequest::Parse(Buffer& buffer) {
    const char CRLF[] = "\r\n";
    if(buffer.ReadableBytes() <= 0) return false;
    while(buffer.ReadableBytes()>0 && m_state!=FINISH) {

    }

    return true;
}