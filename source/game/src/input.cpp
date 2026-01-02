#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"

#include "db.h"
#include "questmanager.h"
#include "fishing.h"
#include "priv_manager.h"
#include "dev_log.h"
#include "../../common/Controls.h"
#include "desc_client.h"
#include "utils.h"

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char* ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1;
	}
	return 0;
}

void ClearAdminPages()
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo* pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}


#ifdef ENABLE_PM_ALL_SEND_SYSTEM
struct SWhisperPacketFunc
{
	const char* c_pszText;

	SWhisperPacketFunc(const char* text) :
		c_pszText(text)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d || !d->GetCharacter())
			return;

		struct packet_bulk_whisper bulk_whisper_pack;
		bulk_whisper_pack.header = HEADER_GC_BULK_WHISPER;
		bulk_whisper_pack.size = sizeof(struct packet_bulk_whisper) + strlen(c_pszText);

		TEMP_BUFFER buf;
		buf.write(&bulk_whisper_pack, sizeof(struct packet_bulk_whisper));
		buf.write(c_pszText, strlen(c_pszText));

		d->Packet(buf.read_peek(), buf.size());
	}
};

void CInputProcessor::SendBulkWhisper(const char* c_pszText)
{
	const DESC_MANAGER::DESC_SET& f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), SWhisperPacketFunc(c_pszText));
}
#endif

bool CInputProcessor::Process(LPDESC lpDesc, const void* c_pvOrig, int iBytes, int& r_iBytesProceed)
{
	const char* c_pData = (const char*)c_pvOrig;

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (thecore_is_shutdowned())
	{
		lpDesc->SetPhase(PHASE_CLOSE);
		return true;
	}

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		BYTE bHeader = (BYTE) * (c_pData);
		const char* c_pszName;

		if (bHeader == 0)
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			sys_err("UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, fd: %d",
				bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			//printdata((BYTE *) c_pvOrig, m_iBufferLeft);
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

		c_pData += iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader = bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

//void CInputProcessor::Handshake(LPDESC d, const char * c_pData)
//{
//    TPacketCGHandshake * p = (TPacketCGHandshake *) c_pData;
//
//    if (d->GetHandshake() != p->dwHandshake)
//    {
//        sys_err("Invalid Handshake on %d", d->GetSocket());
//        d->SetPhase(PHASE_CLOSE);
//    }
//    else
//    {
//        if (d->IsPhase(PHASE_HANDSHAKE))
//        {
//            if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
//            {
//#ifdef _IMPROVED_PACKET_ENCRYPTION_
//                // Prevent duplicate key agreement if already prepared
//                if (!d->IsCipherPrepared())
//                {
//                    d->SendKeyAgreement();
//                }
//                else
//                {
//                    sys_err("Duplicate handshake attempt on socket %d - cipher already prepared", d->GetSocket());
//                    d->SetPhase(PHASE_CLOSE);
//                }
//#else
//                if (g_bAuthServer)
//                    d->SetPhase(PHASE_AUTH);
//                else
//                    d->SetPhase(PHASE_LOGIN);
//#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
//            }
//        }
//        else
//            d->HandshakeProcess(p->dwTime, p->lDelta, true);
//    }
//}

void CInputProcessor::Handshake(LPDESC d, const char * c_pData) {
    TPacketCGHandshake* p = (TPacketCGHandshake*) c_pData;

    // Verificăm dacă handshake-ul este valid
    if (d->GetHandshake() != p->dwHandshake) {
        sys_err("Invalid handshake on socket %d", d->GetSocket());
        d->SetPhase(PHASE_CLOSE);
        return;
    }

    // Procesăm handshake doar dacă suntem în faza corespunzătoare
    if (d->IsPhase(PHASE_HANDSHAKE)) {
        if (d->HandshakeProcess(p->dwTime, p->lDelta, false)) {
#ifdef _IMPROVED_PACKET_ENCRYPTION_
            // Dacă cipher-ul nu e pregătit, trimitem key agreement
            if (!d->IsCipherPrepared()) {
                d->SendKeyAgreement();
            } else {
                sys_err("Duplicate handshake attempt on socket %d - cipher already prepared", d->GetSocket());
                d->SetPhase(PHASE_CLOSE);
            }
#else
            // Flux normal pentru server fără criptare îmbunătățită
            if (g_bAuthServer)
                d->SetPhase(PHASE_AUTH);
            else
                d->SetPhase(PHASE_LOGIN);
#endif
        }
    } else {
        // Dacă handshake-ul vine în altă fază, procesăm ca fallback
        d->HandshakeProcess(p->dwTime, p->lDelta, true);
    }
}

void LoginFailure(LPDESC d, const char* c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG* pkPacketInfo = M2_NEW CPacketInfoCG;
	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if (NULL != m_pPacketInfo)
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	if (bHeader == 10)
		return 0;

	if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) 
		{
			return 0;
		}
		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
#ifdef ENABLE_VOICE_CHAT
	else if (bHeader ==  HEADER_CG_VOICE_CHAT)
	{
		TVoiceChatPacket* p = (TVoiceChatPacket*)c_pData;
		return p->dataSize;
	}
#endif
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else if (bHeader == HEADER_CG_KEY_AGREEMENT)
	{
		d->SendKeyAgreementCompleted();
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}
		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength))
		{
			if (g_bAuthServer)
			{
				d->SetPhase(PHASE_AUTH);
			}
			else
			{
				d->SetPhase(PHASE_LOGIN);
			}
		}
		else
		{
			d->SetPhase(PHASE_CLOSE);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_
	else
		sys_err("Handshake phase does not handle packet %d (fd %d)", bHeader, d->GetSocket());

	return 0;
}

