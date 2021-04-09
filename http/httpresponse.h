#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <string>
#include <string.h>
#include <unordered_map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>

#include "../buffer/buffer.h"

class HTTPResponse {
public:
    HTTPResponse();
    ~HTTPResponse();

    void Init(const std::string& rootDir, std::string& path, bool isKeepAlive, int code);
    void MakeResponse(Buffer& buffer);
    void UnmapFile();

    char * GetFilePtr();
    size_t GetFileLen() const;

private:
    //状态码所对应的状态
    static const std::unordered_map<int, std::string> CODE_TO_STATUS;
    //错误码对应的html页面路径
    static const std::unordered_map<int, std::string> ERRCODE_TO_PATH;
    //文件后缀对应的content-type
    static const std::unordered_map<std::string, std::string> SUFFIX_TO_TYPE;

    int m_code;
    bool m_isKeepAlive;

    std::string m_path;
    std::string m_rootDir;

    char * m_mmFile;
    struct stat m_mmFileStat;

    std::string GetFileType();
    
    void GetErrorHTML();
    void AddResponseLine(Buffer& buffer);
    void AddResponseHeader(Buffer& buffer);
    void AddResponseBody(Buffer& buffer);
    //目录
    void AddDirHTML(Buffer& buffer);
};

#endif