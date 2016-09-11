/**
 * @file    csService.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2016/01/25
 * @version 1.0
 *
 * @brief
 */

#include "csllbc/common/Export.h"

#include "csllbc/comm/csCoder.h"
#include "csllbc/comm/csFacade.h"
#include "csllbc/comm/csService.h"
#include "csllbc/comm/csPacketHandler.h"

int csllbc_Service::_maxSvcId = 1;

LLBC_SpinLock csllbc_Service::_packetDelegatesLock;
csllbc_Service::_PacketDecodeDelegs csllbc_Service::_packetDecodeDelegs;

csllbc_Service::csllbc_Service(Type type,
                               _D::Deleg_Service_EncodePacket encodeDeleg,
                               _D::Deleg_Service_DecodePacket decodeDeleg, 
                               _D::Deleg_Service_PacketHandler handlerDeleg,
                               _D::Deleg_Service_PacketPreHandler preHandlerDeleg,
                               _D::Deleg_Service_PacketUnifyPreHandler unifyPreHandlerDeleg,
                               _D::Deleg_Service_NativeCouldNotFoundDecoderReport notFoundDecoderDeleg)
{
    // Create llbc service.
    _llbcSvc = LLBC_IService::Create(type);

    // Set service Id, csllbc wrapped service don't need serviceId, so simply to set.
    int svcId = LLBC_AtomicFetchAndAdd(&_maxSvcId, 1);
    _llbcSvc->SetId(svcId);

    // Set packet encode delegate.
    _packetEncodeDeleg = encodeDeleg;
    // Set all packet decode/handle about csharp delegates.
    PacketDecodeDelegates *delegs = LLBC_New(PacketDecodeDelegates);
    delegs->decodeDeleg = decodeDeleg;
    delegs->handlerDeleg = handlerDeleg;
    delegs->preHandlerDeleg = preHandlerDeleg;
    delegs->unifyPreHandlerDeleg = unifyPreHandlerDeleg;
    AddPacketDecodeDelegates(svcId, delegs);

    // Create packet handler object.
    _packetHandler = LLBC_New1(csllbc_PacketHandler, notFoundDecoderDeleg);
}

csllbc_Service::~csllbc_Service()
{
    Stop();

    LLBC_Delete(_packetHandler);
    RemovePacketDecodeDelegates(_llbcSvc->GetId());

    LLBC_Delete(_llbcSvc);
}

int csllbc_Service::Start(int pollerCount)
{
    if (IsStarted())
    {
        LLBC_SetLastError(LLBC_ERROR_INITED);
        return LLBC_FAILED;
    }

    return _llbcSvc->Start(pollerCount);
}

void csllbc_Service::Stop()
{
    if (!IsStarted())
        return;

    _llbcSvc->Stop();
}

bool csllbc_Service::IsStarted() const
{
    return _llbcSvc->IsStarted();
}

csllbc_Service::Type csllbc_Service::GetType() const
{
    return _llbcSvc->GetType();
}

int csllbc_Service::GetId() const
{
    return _llbcSvc->GetId();
}

int csllbc_Service::GetFPS() const
{
    return _llbcSvc->GetFPS();
}

int csllbc_Service::SetFPS(int fps)
{
    return _llbcSvc->SetFPS(fps);
}

int csllbc_Service::GetFrameInterval() const
{
    return _llbcSvc->GetFrameInterval();
}

csllbc_Service::DriveMode csllbc_Service::GetDriveMode() const
{
    return _llbcSvc->GetDriveMode();
}

int csllbc_Service::SetDriveMode(DriveMode mode)
{
    return _llbcSvc->SetDriveMode(mode);
}

csllbc_Delegates::Deleg_Service_EncodePacket csllbc_Service::GetEncodePacketDeleg() const
{
    return _packetEncodeDeleg;
}

int csllbc_Service::Listen(const char *ip, uint16 port)
{
    return _llbcSvc->Listen(ip, port);
}

int csllbc_Service::Connect(const char *ip, uint16 port)
{
    return _llbcSvc->Connect(ip, port);
}

int csllbc_Service::AsyncConn(const char *ip, uint16 port)
{
    return _llbcSvc->AsyncConn(ip, port);
}

bool csllbc_Service::IsSessionValidate(int sessionId)
{
    return _llbcSvc->IsSessionValidate(sessionId);
}

int csllbc_Service::RemoveSession(int sessionId, const char *reason)
{
    return _llbcSvc->RemoveSession(sessionId, reason);
}

int csllbc_Service::Send(int sessionId, int opcode, csllbc_Coder *coder, int status)
{
    return _llbcSvc->Send(sessionId, opcode, coder, status);
}

int csllbc_Service::Send(int sessionId, int opcode, const void *bytes, size_t len, int status)
{
    return _llbcSvc->Send(sessionId, opcode, bytes, len, status);
}

int csllbc_Service::Multicast(const LLBC_SessionIdList &sessionIds, int opcode, const void *bytes, size_t len, int status)
{
    return _llbcSvc->Multicast(sessionIds, opcode, bytes, len,status);
}

int csllbc_Service::Broadcast(int opcode, const void *bytes, size_t len, int status)
{
    return _llbcSvc->Broadcast(opcode, bytes, len, status);
}

int csllbc_Service::RegisterFacade(csllbc_Facade *facade)
{
    return _llbcSvc->RegisterFacade(facade);
}

int csllbc_Service::RegisterCoder(int opcode)
{
    csllbc_CoderFactory *coderFactory = LLBC_New(csllbc_CoderFactory);
    if (_llbcSvc->RegisterCoder(opcode, coderFactory) != LLBC_OK)
    {
        LLBC_Delete(coderFactory);
        return LLBC_FAILED;
    }

    return LLBC_OK;
}

int csllbc_Service::Subscribe(int opcode)
{
    return _llbcSvc->Subscribe(opcode, _packetHandler, &csllbc_PacketHandler::Handle);
}

int csllbc_Service::PreSubscribe(int opcode)
{
    return _llbcSvc->PreSubscribe(opcode, _packetHandler, &csllbc_PacketHandler::PreHandle);
}

int csllbc_Service::UnifyPreSubscribe()
{
#if LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE
    return _llbcSvc->UnifyPreSubscribe(_packetHandler, &csllbc_PacketHandler::UnifyPreHandle);
#else
    LLBC_SetLastError(LLBC_ERROR_NOT_IMPL);
    return LLBC_FAILED;
#endif
}

void csllbc_Service::OnSvc(bool fullStack)
{
    _llbcSvc->OnSvc(fullStack);
}

void csllbc_Service::AddPacketDecodeDelegates(int svcId, PacketDecodeDelegates *delegates)
{
    _packetDelegatesLock.Lock();
    _packetDecodeDelegs.insert(std::make_pair(svcId, delegates));
    _packetDelegatesLock.Unlock();
}

csllbc_Service::PacketDecodeDelegates *csllbc_Service::GetPacketDecodeDelegates(int svcId)
{
    _packetDelegatesLock.Lock();
    _PacketDecodeDelegs::iterator it = _packetDecodeDelegs.find(svcId);
    PacketDecodeDelegates *delegs = (it != _packetDecodeDelegs.end() ? it->second : NULL);

    _packetDelegatesLock.Unlock();

    return delegs;
}

void csllbc_Service::RemovePacketDecodeDelegates(int svcId)
{
    _packetDelegatesLock.Lock();
    _PacketDecodeDelegs::iterator it = _packetDecodeDelegs.find(svcId);
    if (it != _packetDecodeDelegs.end())
    {
        LLBC_Delete(it->second);
        _packetDecodeDelegs.erase(it);
    }

    _packetDelegatesLock.Unlock();
}