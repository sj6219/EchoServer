#pragma once

#define UPDATE_SERVER_BIT		0x80
#define UPDATE_NOTICE_BIT		0x40
#define UPDATE_LOAD_BIT			0x20
#define UPDATE_VERSION			2
#define UPDATE_CHANNEL_SIZE			8
#define UPDATE_PORT				50004


#pragma warning( disable: 4200)
#pragma pack( push, 1)

struct U2C_SERVER_HEADER
{
	DWORD dwFileCount;
	DWORD64 qwTotalSize;
};

struct U2C_FILE_HEADER
{
	WORD wSize;
	DWORD dwNo;
	DWORD dwCompressed;
	char  szFileName[0];
};

struct U2C_SERVER_FILE_HEADER
{
	WORD wSize;
	DWORD dwNo;
	DWORD dwCompressed;
	DWORD dwCRC;
	char szFileName[0];
};

struct U2C_NOTICE_HEADER
{
	DWORD dwSize;
};

struct U2C_HEADER
{
	WORD wFileCount[UPDATE_CHANNEL_SIZE];
	DWORD dwTotalSize;
};

struct U2C_VERSION
{
	BYTE byVersion;
	DWORD dwNotice;
	DWORD dwNo[UPDATE_CHANNEL_SIZE];
	DWORD dwAddress;
};

struct C2U_VERSION
{
	BYTE byVersion;
	DWORD dwNotice;
	DWORD dwNo[UPDATE_CHANNEL_SIZE];
	char szFileName[0];
};

struct C2U_SERVER_VERSION
{
	BYTE byVersion;
	BYTE byType;
	DWORD dwNotice;
	DWORD dwNo;
};

#pragma pack( pop)