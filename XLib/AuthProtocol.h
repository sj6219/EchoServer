#pragma once

#define AUTH_PORT	50001
#define AUTH_CMD_PORT 50002
#define AUTH_VERSION 5

enum S2A_PROTOCOL
{
	S2A_EVENT,
	S2A_RESERVED0,
	S2A_RESERVED1,
	S2A_RESERVED2,
	S2A_RESERVED3,
	S2A_RESERVED4,
	S2A_RESERVED5,
	S2A_RESERVED6,
	S2A_RESERVED7,
	S2A_RESERVED8,
	S2A_RESERVED9,

	S2A_SYNC,
	S2A_VERSION,
	S2A_SVRINFO,
	S2A_CLOSE,
	S2A_LOGIN,
	S2A_LOGOUT,
	S2A_RELOGIN,
	S2A_USER,
	S2A_USERALL,
	S2A_CLEARLOGIN,
	S2A_NEWPC,
	S2A_BLOCK,
	S2A_FREE,
	S2A_REQUESTCOPY,
	S2A_ANS_REQUESTCOPY,
	S2A_SPECIALITEM,
	S2A_R_NEWPC,
	S2A_LOGIN_NEXON,
	S2A_NEWGUILD,
	S2A_REMOVE_GUILDNAME,
	S2A_NXSHOP,
	S2A_R_CMD_NX_GM_DEPRECATED,	// 
	S2A_SYSEVENT_CODE,
	S2A_INFO,
	S2A_CMD_ANSWER,

	S2A_END,
};

enum A2S_PROTOCOL
{
	A2S_VERSION,
	A2S_R_LOGIN,
	A2S_R_USER,
	A2S_R_USERALL,
	A2S_KICKOUT,
	A2S_R_NEWPC,
	A2S_R_BLOCK,
	A2S_R_FREE,
	A2S_R_EVENT,
	A2S_REQUESTCOPY,
	A2S_ANS_REQUESTCOPY,
	A2S_NXAUTH,
	A2S_SPECIALITEM,
	A2S_NOTICE,
	A2S_R_NEWGUILD,
	A2S_NXSHOP,
	A2S_CMD_CREATEITEM, // cstool�� 
	A2S_CMD_DELETEITEM,	// cstool�� 
	A2S_SYSEVENT_CODE,
	A2S_CMD_RESETPASS,
	A2S_CMD_CREATEITEM_EX, // cstool�� 
	A2S_CMD_CREATEITEM_DESC, // cstool�� 

	A2S_END
};

#pragma pack(push, 1)

struct AUTHPACKET
{
	WORD wSize;
	BYTE byType;
	char data[0];
};

#pragma pack(pop)
