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
    //目录
    if(S_ISDIR(m_mmFileStat.st_mode)) 
        return "text/html; charset=utf-8";
    size_t idx = m_path.find_last_of('.');
    //没找到
    if(idx == std::string::npos)
        return "text/plain; charset=utf-8";
    std::string suffix = m_path.substr(idx);
    if(SUFFIX_TO_TYPE.count(suffix))
        return SUFFIX_TO_TYPE.find(suffix)->second;
    return "text/plain; charset=utf-8";
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
        buffer.Append("Keep-Alive: max=6, timeout=120\r\n");
    }
    else buffer.Append("close\r\n");
    //Content-type
    buffer.Append("Content-type: " + GetFileType() + "\r\n");
    //Content-length -1表自动计算
    //注意最后接一空行
    buffer.Append("Content-length: -1\r\n\r\n");
}

void HTTPResponse::AddResponseBody(Buffer& buffer) {
    //文件 包括400.html 403.html 404.html 
    if(S_ISREG(m_mmFileStat.st_mode)) {
        //打开文件 建立映射->将文件映射到内存提高文件的访问速度 
        int fd = open((m_rootDir+m_path).data(), O_RDONLY);
        //MAP_PRIVATE 建立一个写入时拷贝的私有映射
        m_mmFile = (char *)mmap(0, m_mmFileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
    }
    //目录
    else {
        AddDirHTML(buffer);
    }
}

void HTTPResponse::AddDirHTML(Buffer& buffer) {
    //将请求"test"文件夹转换为请求"test/"
    if('/' != m_path[m_path.size()-1])
        m_path.push_back('/');

    buffer.Append("<!DOCTYPE html><html>\n");
    buffer.Append("<head><title>文件服务器</title></head>\n");

    //拼一个html页面 <table></table>
    buffer.Append("<body><h1>当前目录: " + m_path + "</h1><table>\n");
    //表头
    buffer.Append("<tr><th>Name</th><th>Size</th></tr>\n");
    //目录项二级指针
    struct dirent** ptr;
    char dirPath[1024] = {0};
    strcpy (dirPath, (m_rootDir+m_path).data());
    int num = scandir(dirPath, &ptr, NULL, alphasort);

    char filepath[1024] = {0};
    //遍历
    for(int i=0; i<num; i++) {
        const char * name = ptr[i]->d_name;
        
        //拼接文件的完整路径
        sprintf(filepath, "%s%s", dirPath, name);

        //获取文件信息
        struct stat st;
        stat(filepath, &st);
        
        char content[1024] = {0};
        //文件
        if(S_ISREG(st.st_mode)) {
            sprintf(content, "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>\n",
                    name, name, (long)st.st_size);
        }
        //目录
        else if(S_ISDIR(st.st_mode)) {
            sprintf(content, "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>\n",
                    name, name, (long)st.st_size);
        }
        buffer.Append(content);
    }
    buffer.Append("</table></body></html>");
}