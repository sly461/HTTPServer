#include "buffer.h"


Buffer::Buffer(int initBufferSize):
    m_buffer(initBufferSize), m_readPos(0), m_writePos(0)
{

}

char *Buffer::BeginPtr() {
    return &*m_buffer.begin();
}

const char *Buffer::BeginPtr() const {
    return BeginPtr();
}