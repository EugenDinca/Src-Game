#include "stdafx.h"
#include "Peer.h"
#include "ItemIDRangeManager.h"

CPeer::CPeer()
{
	m_state = 0;
	m_bChannel = 0;
	m_dwHandle = 0;
	m_dwUserCount = 0;
	m_wListenPort = 0;
	m_wP2PPort = 0;

	memset(m_alMaps, 0, sizeof(m_alMaps));

	m_itemRange.dwMin = m_itemRange.dwMax = m_itemRange.dwUsableItemIDMin = 0;
	m_itemSpareRange.dwMin = m_itemSpareRange.dwMax = m_itemSpareRange.dwUsableItemIDMin = 0;
}

CPeer::~CPeer()
{
	Close();
}

void CPeer::OnAccept()
{
	m_state = STATE_PLAYING;

	static DWORD current_handle = 0;
	m_dwHandle = ++current_handle;
}

void CPeer::OnConnect()
{
	m_state = STATE_PLAYING;
}

void CPeer::OnClose()
{
	m_state = STATE_CLOSE;
	CItemIDRangeManager::instance().UpdateRange(m_itemRange.dwMin, m_itemRange.dwMax);

	m_itemRange.dwMin = 0;
	m_itemRange.dwMax = 0;
	m_itemRange.dwUsableItemIDMin = 0;
}

DWORD CPeer::GetHandle()
{
	return m_dwHandle;
}

DWORD CPeer::GetUserCount()
{
	return m_dwUserCount;
}

void CPeer::SetUserCount(DWORD dwCount)
{
	m_dwUserCount = dwCount;
}

bool CPeer::PeekPacket(int& iBytesProceed, BYTE& header, DWORD& dwHandle, DWORD& dwLength, const char** data)
{
	if (GetRecvLength() < iBytesProceed + 9)
		return false;

	const char* buf = (const char*)GetRecvBuffer();
	buf += iBytesProceed;

	header = *(buf++);

	dwHandle = *((DWORD*)buf);
	buf += sizeof(DWORD);

	dwLength = *((DWORD*)buf);
	buf += sizeof(DWORD);

	if (iBytesProceed + dwLength + 9 > (DWORD)GetRecvLength())
	{
		return false;
	}

	*data = buf;
	iBytesProceed += dwLength + 9;
	return true;
}

void CPeer::EncodeHeader(BYTE header, DWORD dwHandle, DWORD dwSize)
{
	HEADER h;
	h.bHeader = header;
	h.dwHandle = dwHandle;
	h.dwSize = dwSize;

	Encode(&h, sizeof(HEADER));
}

void CPeer::EncodeReturn(BYTE header, DWORD dwHandle)
{
	EncodeHeader(header, dwHandle, 0);
}

int CPeer::Send()
{
	if (m_state == STATE_CLOSE)
		return -1;

	return (CPeerBase::Send());
}

void CPeer::SetP2PPort(WORD wPort)
{
	m_wP2PPort = wPort;
}

void CPeer::SetMaps(long* pl)
{
	thecore_memcpy(m_alMaps, pl, sizeof(m_alMaps));
}

void CPeer::SendSpareItemIDRange()
{
	if (m_itemSpareRange.dwMin == 0 || m_itemSpareRange.dwMax == 0 || m_itemSpareRange.dwUsableItemIDMin == 0)
	{
		EncodeHeader(HEADER_DG_ACK_SPARE_ITEM_ID_RANGE, 0, sizeof(TItemIDRangeTable));
		Encode(&m_itemSpareRange, sizeof(TItemIDRangeTable));
	}
	else
	{
		SetItemIDRange(m_itemSpareRange);

		if (SetSpareItemIDRange(CItemIDRangeManager::instance().GetRange()) == false)
		{
			m_itemSpareRange.dwMin = m_itemSpareRange.dwMax = m_itemSpareRange.dwUsableItemIDMin = 0;
		}

		EncodeHeader(HEADER_DG_ACK_SPARE_ITEM_ID_RANGE, 0, sizeof(TItemIDRangeTable));
		Encode(&m_itemSpareRange, sizeof(TItemIDRangeTable));
	}
}

bool CPeer::SetItemIDRange(TItemIDRangeTable itemRange)
{
	if (itemRange.dwMin == 0 || itemRange.dwMax == 0 || itemRange.dwUsableItemIDMin == 0) return false;

	m_itemRange = itemRange;
	return true;
}

bool CPeer::SetSpareItemIDRange(TItemIDRangeTable itemRange)
{
	if (itemRange.dwMin == 0 || itemRange.dwMax == 0 || itemRange.dwUsableItemIDMin == 0) return false;

	m_itemSpareRange = itemRange;
	return true;
}

bool CPeer::CheckItemIDRangeCollision(TItemIDRangeTable itemRange)
{
	if (m_itemRange.dwMin < itemRange.dwMax && m_itemRange.dwMax > itemRange.dwMin)
	{
		sys_err("ItemIDRange: Collision!! this %u ~ %u check %u ~ %u",
			m_itemRange.dwMin, m_itemRange.dwMax, itemRange.dwMin, itemRange.dwMax);
		return false;
	}

	if (m_itemSpareRange.dwMin < itemRange.dwMax && m_itemSpareRange.dwMax > itemRange.dwMin)
	{
		sys_err("ItemIDRange: Collision with spare range this %u ~ %u check %u ~ %u",
			m_itemSpareRange.dwMin, m_itemSpareRange.dwMax, itemRange.dwMin, itemRange.dwMax);
		return false;
	}

	return true;
}