/**
 * 缓冲区
**/
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>
#include <string>

class Buffer {
public:
    Buffer(int initBufferSize=1024);
    ~Buffer() = default;
    
    size_t WritableBytes() const;
    size_t ReadableBytes() const;

    char *BeginWritePtr();
    const char *BeginWritePtr() const;
    char *BeginReadPtr();
    const char *BeginReadPtr() const;

    void HasWritten(size_t len);
    void HasRead(size_t len);

    void Append(const char* str, size_t len);
    void Append(const std::string& str);

private:
    std::vector<char> m_buffer;
    //是否应该设置atomic？
    size_t m_readPos;
    size_t m_writePos;

    char *BeginPtr();
    const char *BeginPtr() const;

    //拓展空间
    void ExpandSpace(size_t len);
};


#endif