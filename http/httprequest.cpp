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

bool HTTPRequest::ParseRequestLine(const std::string& line) {
    //正则表达式 ^ $分别匹配开头和结尾
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, pattern)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version = subMatch[3];
        m_state = REQUEST_HEADER;
        return true;
    }
    return false;
}

void HTTPRequest::ParseRequestHeader(const std::string& line) {
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, pattern)) {
        m_header[subMatch[1]] = subMatch[2];
    }
    else m_state = REQUEST_BODY;
}

void HTTPRequest::ParsePath() {
    //去掉首字母'/'
    if(m_path == "/") m_path = "./";
    else m_path.erase(0, 1);
}

bool HTTPRequest::Parse(Buffer& buffer) {
    const char CRLF[] = "\r\n";
    if(buffer.ReadableBytes() <= 0) return false;
    while(buffer.ReadableBytes()>0 && m_state!=FINISH) {
        const char * lineEnd = std::search(buffer.BeginReadPtr(), const_cast<const Buffer&>(buffer).BeginWritePtr(), CRLF, CRLF+2);
        std::string line(buffer.BeginReadPtr(), lineEnd);
        switch (m_state)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine(line))
                return false;
            ParsePath();
            break;
        case REQUEST_HEADER:
            ParseRequestHeader(line);
            if(buffer.ReadableBytes() <= 2)
                m_state = FINISH;
            break;
        case REQUEST_BODY:
            //TODO
            break;
        default:
            break;
        }
        if(lineEnd == buffer.BeginWritePtr()) break;
        buffer.HasRead(line.size() + 2);
    }
    return true;
}