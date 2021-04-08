#include "httpresponse.h"

HTTPResponse::HTTPResponse():
    m_code(-1), m_isKeepAlive(false),
    m_path(""), m_rootDir(""),
    m_mmFile(nullptr)
{
    bzero(&m_mmFileStat, sizeof(m_mmFileStat));
}
    
HTTPResponse::~HTTPResponse() {

}

void HTTPResponse::Init(const std::string& rootDir, std::string& path, bool isKeepAlive, int code) {

}

void HTTPResponse::MakeResponse(Buffer& buffer) {

}
    
void HTTPResponse::UnmapFile() {

}