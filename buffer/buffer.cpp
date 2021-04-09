#include "buffer.h"


Buffer::Buffer(int initBufferSize):
    m_buffer(initBufferSize), m_readPos(0), m_writePos(0)
{

}

char *Buffer::BeginPtr() {
    return &*m_buffer.begin();
}

const char *Buffer::BeginPtr() const {
    return &*m_buffer.begin();
}

void Buffer::ExpandSpace(size_t len) {
    if(m_readPos+WritableBytes() < len) {
        m_buffer.resize(m_writePos+len+1);
    }
    //已经读取的空间+未写的空间 >= len
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr()+m_readPos, BeginPtr()+m_writePos, BeginPtr());
        m_readPos = 0;
        m_writePos = m_readPos + readable;
    }
}

size_t Buffer::WritableBytes() const {
    return m_buffer.size() - m_writePos;
}

size_t Buffer::ReadableBytes() const {
    return m_writePos - m_readPos;
}

char *Buffer::BeginWritePtr() {
    return BeginPtr() + m_writePos;
}

const char *Buffer::BeginWritePtr() const {
    return BeginPtr() + m_writePos;
}

char *Buffer::BeginReadPtr() {
    return BeginPtr() + m_readPos;
}

const char *Buffer::BeginReadPtr() const {
    return BeginPtr() + m_readPos;
}

void Buffer::HasWritten(size_t len) {
    m_writePos += len;
}

void Buffer::HasRead(size_t len) {
    m_readPos += len;
}

void Buffer::Append(const char* str, size_t len) {
    if(WritableBytes() < len) ExpandSpace(len);
    std::copy(str, str+len, BeginWritePtr());
    HasWritten(len);
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.size());
}