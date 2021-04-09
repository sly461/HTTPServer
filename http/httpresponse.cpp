#include "httpresponse.h"

const std::unordered_map<std::string, std::string> 
HTTPResponse::SUFFIX_TO_TYPE = {
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".png", "image/png"},
    {".css", "text/css"},
    {".au", "audio/basic"},
    {".wav", "audio/wav"},
    {".avi", "video/x-msvideo"},
    {".mov", "video/quicktime"},
    {".qt", "video/quicktime"},
    {".mpeg", "video/mpeg"},
    {".mpe", "video/mpeg"},
    {".vrml", "model/vrml"},
    {".wrl", "model/vrml"},
    {".midi", "audio/midi"},
    {".mid", "audio/midi"},
    {".mp3", "audio/mpeg"},
    {".ogg", "application/ogg"},
    {".pac", "application/x-ns-proxy-autoconfig"}
};

const std::unordered_map<int, std::string> HTTPResponse::CODE_TO_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"}
};

const std::unordered_map<int, std::string> HTTPResponse::ERRCODE_TO_PATH = {
    {400, "400.html"},
    {403, "403.html"},
    {404, "404.html"}
};

HTTPResponse::HTTPResponse():
    m_code(-1), m_isKeepAlive(false),
    m_path(""), m_rootDir(""),
    m_mmFile(nullptr)
{
    bzero(&m_mmFileStat, sizeof(m_mmFileStat));
}
    
HTTPResponse::~HTTPResponse() {
    UnmapFile();
}

void HTTPResponse::Init(const std::string& rootDir, std::string& path, bool isKeepAlive, int code) {
    if(m_mmFile) UnmapFile();
    m_rootDir = rootDir;
    m_path = path;
    m_isKeepAlive = isKeepAlive;
    m_code = code;
    bzero(&m_mmFileStat, sizeof(m_mmFileStat));
}

void HTTPResponse::MakeResponse(Buffer& buffer) {
    do {
        if(m_code == 400) break;
        int ret = stat((m_rootDir+m_path).data(), &m_mmFileStat);
        // 404
        if(ret < 0) 
            m_code = 404;
        //其他组成员没有读权限
        else if(!(m_mmFileStat.st_mode & S_IROTH))
            m_code = 403;
        else if(m_code == -1)
            m_code = 200;
    } while(0);
    GetErrorHTML();
    AddResponseLine(buffer);
    AddResponseHeader(buffer);
    AddResponseBody(buffer);
    //std::cout << m_rootDir << " " << m_path <<  " " <<m_isKeepAlive << " " << m_code <<std::endl;
}
    
void HTTPResponse::UnmapFile() {
    if(m_mmFile) {
        munmap(m_mmFile, m_mmFileStat.st_size);
        m_mmFile = nullptr;
    }
}

char * HTTPResponse::GetFilePtr() {
    return m_mmFile;
}

size_t HTTPResponse::GetFileLen() const {
    return m_mmFileStat.st_size;
}

std::string HTTPResponse::GetFileType() {
    size_t idx = m_path.find_last_of('.');
    //没找到
    if(idx == std::string::npos)
        return "text/plain";
    std::string suffix = m_path.substr(idx);
    if(SUFFIX_TO_TYPE.count(suffix))
        return SUFFIX_TO_TYPE.find(suffix)->second;
    return "text/plain";
}

void HTTPResponse::GetErrorHTML() {
    if(ERRCODE_TO_PATH.count(m_code)) {
        m_path = ERRCODE_TO_PATH.find(m_code)->second;
        stat((m_rootDir+m_path).data(), &m_mmFileStat);
    }
}

void HTTPResponse::AddResponseLine(Buffer& buffer) {
    std::string status;
    if(CODE_TO_STATUS.count(m_code))
        status = CODE_TO_STATUS.find(m_code)->second;
    std::string line = "HTTP/1.1 " + std::to_string(m_code) + " " + status + "\r\n";
    buffer.Append(line);
}

void HTTPResponse::AddResponseHeader(Buffer& buffer) {
    //Connection
    buffer.Append("Connection: ");
    if(m_isKeepAlive) {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else buffer.Append("close\r\n");
    //Content-type
    buffer.Append("Content-type: " + GetFileType() + "\r\n");
    //Content-length -1表自动计算
    buffer.Append("Content-length: -1\r\n");
}

void HTTPResponse::AddResponseBody(Buffer& buffer) {

}