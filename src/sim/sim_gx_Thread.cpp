#include <cstring>

#include "simulator/sim.hpp"
#include "simulator/sim_gx_Thread.hpp"
#include "simulator/sim_gx_Thread.h"
#include "simulator/sim_gx_GlRenderer.hpp"
#include "simulator/sim_gx_CommandProcessor.hpp"
#include "simulator/sim_gx_State.hpp"
#include "simulator/sim_gx_TextureManager.hpp"
#include "simulator/sim_MessageQueue.hpp"

#include <SDL2/SDL.h>

#define COMMAND_PROCESSOR_DEBUG 0

static SIM::GX::CommandProcessor sCommandProcessor = SIM::GX::CommandProcessor();
static SIM::MessageQueue sMessageQueue = SIM::MessageQueue<SIM::GX::ThreadMessage>(512 * 1024);
static SDL_Thread * sGxMainThread;
static SDL_sem* sGxRenderContextSemaphore;
static SDL_sem* sGxDrawDoneSemaphore;

static bool sInDisplayList = false;
static u32 sDisplayListBytes = 0;
static u32 sDisplayListBytesWritten;
static u8 * sDisplayListPtr = nullptr;
static SDL_mutex * sFifoMutex;


namespace SIM::GX {
void Init() {
    InitGlobalState();

    sGxMainThread = SDL_CreateThread(MainThread, "SIM::GX", nullptr);
    sFifoMutex = SDL_CreateMutex();
}

int MainThread(void * arg) {

    SIM::AcquireRenderContext();
    sGxRenderContextSemaphore = SDL_CreateSemaphore(0);
    sGxDrawDoneSemaphore = SDL_CreateSemaphore(0);
    while(true) {
        //Wait for messages
        auto msg = sMessageQueue.ReceiveMessage();

        switch(msg.mType) {
            case ThreadMessageType::Fifo:
                if(msg.mFifo.fifoData) {
                    sCommandProcessor.ProcessFifoData(msg.mFifo.fifoData, msg.mFifo.fifoDataLen, std::endian::native);
                    delete(msg.mFifo.fifoData);
                    msg.mFifo.fifoData = nullptr;
                }
                break;
            case ThreadMessageType::SetVertexArray:
                {
                    u8 * msgData = msg.mData;
                    SIM::GX::VertexArray* vtxArray = (SIM::GX::VertexArray*)msgData;
                    SIM::GX::GetGlobalState().SetVertexArray(vtxArray->attribute, *vtxArray);
                } break;
            case ThreadMessageType::InitTexObj:
                {
                    // Deprecated
                    //auto& manager = SIM::GX::TextureManager::GetInstance();
//
                    //manager.InitTexObj(msg.mInitTexObj.obj, 
                    //                   msg.mInitTexObj.imagePtr, 
                    //                   msg.mInitTexObj.width, 
                    //                   msg.mInitTexObj.height, 
                    //                   msg.mInitTexObj.format, 
                    //                   msg.mInitTexObj.wrapS, 
                    //                   msg.mInitTexObj.wrapT, 
                    //                   msg.mInitTexObj.mipmap);
                } break;
            case ThreadMessageType::LoadTexObj:
                {
                    // deprecated
                    //auto& manager = SIM::GX::TextureManager::GetInstance();

                    //manager.LoadTexObj(msg.mLoadTexObj.obj, msg.mLoadTexObj.map);
                } break;
            case ThreadMessageType::TakeRenderContext:
                {
                    //Release render context
                    SIM::ReleaseRenderContext();

                    //Call the callback in the message
                    u64 * dataPtr = (u64*)msg.mData;
                    SDL_sem* semaphore = (SDL_sem*)(*dataPtr);
                    if(semaphore) {
                        //Notify sending thread
                        SDL_SemPost(semaphore);
                    }

                    // Pause until the context is given back
                    SDL_SemWait(sGxRenderContextSemaphore);

                    //Reacquire context
                    SIM::AcquireRenderContext();
                }
                break;
            case ThreadMessageType::FlushGlBuffer: {
                auto& renderer = GetGlRenderer();
                renderer.FlushRenderVerts();
            } break;
            default:
                break;
        }
    }

    return 0;
}

static u8 * sInternalFifoBuffer = nullptr; // These buffers will be allocated by the calling thread, and freed by GX commandprocessor
static constexpr auto InternalFifoBufferSize = 600;
static constexpr auto InternalFifoBufferSendThreshold = 512;
static u32 sInternalFifoBufferPos = 0;

void FlushFifoBuffer() {
    SDL_LockMutex(sFifoMutex);
    if(sInternalFifoBufferPos > 0) {
        ThreadMessage msg;
        msg.mType = ThreadMessageType::Fifo;
        msg.mFifo.fifoData = sInternalFifoBuffer;
        msg.mFifo.fifoDataLen = sInternalFifoBufferPos;

        sMessageQueue.SendMessage(msg);
        sInternalFifoBufferPos = 0;
        sInternalFifoBuffer = new u8[InternalFifoBufferSize];
    }
    SDL_UnlockMutex(sFifoMutex);
}

void FlushGlBuffer() {
    ThreadMessage msg;
    msg.mType = ThreadMessageType::FlushGlBuffer;
    sMessageQueue.SendMessage(msg);
}

template <typename T>
void SendFifoMessage(T data) {
    size_t dataLen = sizeof(T);

    if(sInDisplayList) {
        size_t displayListBytes = std::min<size_t>(sDisplayListBytes, dataLen);
        std::memcpy(sDisplayListPtr, &data, displayListBytes);
        sDisplayListPtr += displayListBytes;
        sDisplayListBytes -= displayListBytes;
        sDisplayListBytesWritten+=displayListBytes;

        if(sDisplayListBytes <= 0) {
            sInDisplayList = false;
        }

        return;
    }

    if(dataLen > 8) {
        //Message too big! (todo maybe increase it)
        return;
    }

    #if COMMAND_PROCESSOR_DEBUG
    u8 * dataPtr = (u8*)(&data);
    sCommandProcessor.ProcessFifoData(dataPtr, dataLen, std::endian::native);
    return;
    #endif
    //#else
    //sMessageQueue.SendMessage(msg);
    //#endif


    // Add to the internal message buffer
    SDL_LockMutex(sFifoMutex);
    if(sInternalFifoBuffer == nullptr) {
        sInternalFifoBuffer = new u8[InternalFifoBufferSize];
    }

    std::memcpy(&sInternalFifoBuffer[sInternalFifoBufferPos], &data, dataLen);
    sInternalFifoBufferPos+= dataLen;

    if(sInternalFifoBufferPos >= InternalFifoBufferSendThreshold) {
        FlushFifoBuffer();
    }
    SDL_UnlockMutex(sFifoMutex);
    //msg.mDataLen = dataLen;
}

void SendThreadMessage(ThreadMessage& msg) {
    sMessageQueue.SendMessage(msg);
}

//Take the render context from the GX Thread
//Pause the GX thread until the render context is given back
void TakeRenderContext() {
    SDL_sem* semaphore = SDL_CreateSemaphore(0);
    ThreadMessage msg;
    msg.mType = ThreadMessageType::TakeRenderContext;
    std::memcpy(msg.mData, &semaphore, sizeof(void*));

    sMessageQueue.SendMessage(msg);

    SDL_SemWait(semaphore);
    SDL_DestroySemaphore(semaphore);
}

// Give the render context back to the GX thread and resume it
void GiveRenderContext() {
    SDL_SemPost(sGxRenderContextSemaphore);
}

bool IsThreadDone() {
    return sMessageQueue.empty();
}

void WaitDrawDone() {
    SDL_SemWait(sGxDrawDoneSemaphore);
}

void SetDrawDone() {
    SDL_SemPost(sGxDrawDoneSemaphore);
}


}

// C APIs for GX Thread/Fifo

void SIM_GX_Fifo_SendU8(u8 data) {
    SIM::GX::SendFifoMessage<u8>(data);
}

void SIM_GX_Fifo_SendU16(u16 data) {
    SIM::GX::SendFifoMessage<u16>(data);
}

void SIM_GX_Fifo_SendS16(s16 data) {
    SIM::GX::SendFifoMessage<s16>(data);
}

void SIM_GX_Fifo_SendU32(u32 data) {
    SIM::GX::SendFifoMessage<u32>(data);
}

void SIM_GX_Fifo_SendF32(f32 data) {
    SIM::GX::SendFifoMessage<f32>(data);
}

void SIM_GX_Fifo_SendU64(u64 data) {
    SIM::GX::SendFifoMessage<u64>(data);
}

void SIM_GX_BeginDisplayList(u8 * ptr, u32 size) {
    sDisplayListPtr = ptr;
    sDisplayListBytes = size;
    sInDisplayList = true;
    sDisplayListBytesWritten = 0;
    
    // Display lists created at runtime in this way should always be in native endian
    SIM::GX::GetGlobalState().AddNativeEndianDisplayList(ptr);
}

u32 SIM_GX_EndDisplayList() {
    sInDisplayList = false;
    return sDisplayListBytesWritten;
}

void SIM_GX_CommandProcessor_SetVertexArray(GXAttr attr, void * ptr, int stride) {
    SIM::GX::ThreadMessage msg;
    msg.mType = SIM::GX::ThreadMessageType::SetVertexArray;
    msg.mDataLen = sizeof(SIM::GX::VertexArray);
    msg.mVertexArray.attribute = attr;
    msg.mVertexArray.mArrayPtr = ptr;
    msg.mVertexArray.mStride = stride;
    sMessageQueue.SendMessage(msg);
}

void SIM_GX_FlushFifo() {
    SIM::GX::FlushFifoBuffer();
}

void SIM_GX_WaitDrawDone() {
    //SIM::GX::WaitDrawDone();
}