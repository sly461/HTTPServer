/**
 * 缓冲区
**/
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>

class Buffer {
public:
    Buffer(int initBufferSize=1024);
    ~Buffer() = default;


private:
    std::vector<char> m_buffer;
    //是否应该设置atomic？
    size_t m_readPos;
    size_t m_writePos;

    char *BeginPtr();
    const char *BeginPtr() const;

};


#endif