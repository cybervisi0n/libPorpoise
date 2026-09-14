#ifndef LIBPORPOISE_SIM_GX_THREAD_HPP
#define LIBPORPOISE_SIM_GX_THREAD_HPP

#include <dolphin/types.h>
#include <SDL2/SDL_mutex.h>
#include <simulator/sim_gx_State.hpp>

namespace SIM::GX {

enum class ThreadMessageType {
 Fifo,
 SetVertexArray,
 InitTexObj,
 LoadTexObj,
 TakeRenderContext,
 Count
};

struct FifoMessage {
    u8 * fifoData;
    u32 fifoDataLen;
};

struct InitTexObjMessage {
    GXTexObj* obj;
    void * imagePtr;
    u16 width;
    u16 height;
    GXTexFmt format;
    GXTexWrapMode wrapS;
    GXTexWrapMode wrapT;
    u8 mipmap;
};

struct LoadTexObjMessage {
    GXTexObj* obj;
    GXTexMapID map;
};

struct ThreadMessage {
 ThreadMessage(){};
 ThreadMessageType mType;
 union {
    void * mPtr;
    FifoMessage mFifo;
    VertexArray mVertexArray;
    InitTexObjMessage mInitTexObj;
    LoadTexObjMessage mLoadTexObj;
    u8 mData[16];
 };
 size_t mDataLen;
};

void Init();
int MainThread(void * arg);
void FlushFifoBuffer();
template <typename T>
void SendFifoMessage(T data);
void SendThreadMessage(ThreadMessage& msg);
bool IsThreadDone();
void TakeRenderContext();
void GiveRenderContext();

}

#endif
