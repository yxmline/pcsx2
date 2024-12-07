// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "TCP_Options.h"
#include "DEV9/PacketReader/NetLib.h"

namespace PacketReader::IP::TCP
{
	TCPopMSS::TCPopMSS(u16 mss)
		: maxSegmentSize{mss}
	{
	}
	TCPopMSS::TCPopMSS(const u8* data, int offset)
	{
		offset += 2;
		NetLib::ReadUInt16(data, &offset, &maxSegmentSize);
	}
	void TCPopMSS::WriteBytes(u8* buffer, int* offset) const
	{
		NetLib::WriteByte08(buffer, offset, GetCode());
		NetLib::WriteByte08(buffer, offset, GetLength());

		NetLib::WriteUInt16(buffer, offset, maxSegmentSize);
	}

	TCPopWS::TCPopWS(u8 ws)
		: windowScale{ws}
	{
	}
	TCPopWS::TCPopWS(const u8* data, int offset)
	{
		offset += 2;
		NetLib::ReadByte08(data, &offset, &windowScale);
	}
	void TCPopWS::WriteBytes(u8* buffer, int* offset) const
	{
		NetLib::WriteByte08(buffer, offset, GetCode());
		NetLib::WriteByte08(buffer, offset, GetLength());

		NetLib::WriteByte08(buffer, offset, windowScale);
	}

	TCPopTS::TCPopTS(u32 senderTS, u32 echoTS)
		: senderTimeStamp{senderTS}
		, echoTimeStamp{echoTS}
	{
	}
	TCPopTS::TCPopTS(const u8* data, int offset)
	{
		offset += 2;
		NetLib::ReadUInt32(data, &offset, &senderTimeStamp);
		NetLib::ReadUInt32(data, &offset, &echoTimeStamp);
	}
	void TCPopTS::WriteBytes(u8* buffer, int* offset) const
	{
		NetLib::WriteByte08(buffer, offset, GetCode());
		NetLib::WriteByte08(buffer, offset, GetLength());

		NetLib::WriteUInt32(buffer, offset, senderTimeStamp);
		NetLib::WriteUInt32(buffer, offset, echoTimeStamp);
	}
} // namespace PacketReader::IP::TCP
