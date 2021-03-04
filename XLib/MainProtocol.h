#pragma once

#define SERVER_PORT	30001
#define PROTOCOL_VERSION 6

enum S2C_PROTOCOL
{
	S2C_CODE,	// 0
	S2C_ALIVE,
	S2C_CLOSE,
	S2C_R_LOGIN,
	S2C_R_NEWPC,
	S2C_MYPCINFO,
	S2C_R_LOAD,
	S2C_R_START,
	S2C_R_RESTART,
	S2C_R_GAMEEXIT,
	S2C_ATTRIBUTE,	// 10
	S2C_R_CHAT,
	S2C_NOTICE,
	S2C_CREATEPC,
	S2C_CREATENPC,
	S2C_REMOVEPC,
	S2C_REMOVENPC,
	S2C_PCMOVE,
	S2C_PCMOVEEND,
	S2C_PCMOVEPREV,
	S2C_NPCMOVE,	// 20
	S2C_NPCMOVEEND,
	S2C_SKILL,
	S2C_ACTION,
	S2C_MESSAGE,
	S2C_SCRIPT,
	S2C_INVENINFO,
	S2C_ITEM,
	S2C_FRIEND,
	S2C_TARGET,
	S2C_UPDATEATTR,
	S2C_TELEPORT,	// 30
	S2C_PARTY,
	S2C_GSTATE,
	S2C_MSTATE,
	S2C_BSTATE,
	S2C_SETLOOTER,
	S2C_BUFF,
	S2C_EFFECT,
	S2C_CHECKDATA,
	S2C_LOADPC_ANS,
	S2C_PCMOVEFORCE,	// 40
	S2C_QUEST,
	S2C_HOLE,
	S2C_MAIL,
	S2C_RESETDIR,
	S2C_SHORTCUT,
	S2C_SETBIND,
	S2C_MAPINFO,
	S2C_COOLDOWN,
	S2C_CRAFT,
	S2C_SPECIALTY,	// 50
	S2C_NPROTECT,
	S2C_GUILD,
	S2C_AUCTION,
	S2C_TRIGGER,
	S2C_TODAYGAS,
	S2C_NXSHOP,
	S2C_ETC,
	S2C_SCRAMBLE,
	S2C_EVENT,
	S2C_SYSEVENTCODE,	// 60
	S2C_PASSWORD,
	S2C_CHAOS,
	S2C_MINIGAME,

	S2C_END
};

enum C2S_PROTOCOL
{
	C2S_R_ALIVE,	// 0
	C2S_CONNECT,
	C2S_R_CODE,
	C2S_LOGIN,
	C2S_NEWPC,
	C2S_DELETEPC,
	C2S_COPYPC,
	C2S_LOADPC,
	C2S_R_NPROTECT,
	C2S_PASSWORD,
	C2S_RESERVED3,	// 10
	C2S_RESERVED4,
	C2S_RESERVED5,
	C2S_START,
	C2S_RESTART,
	C2S_GAMEEXIT,
	C2S_PCMOVE,
	C2S_PCMOVEEND,
	C2S_ATTACK,
	C2S_CHAT,
	C2S_ITEM,	// 20
	C2S_FRIEND,
	C2S_MAIL,
	C2S_PARTY,
	C2S_AUCTION,
	C2S_SCRIPT,
	C2S_ACTION,
	C2S_TARGET,
	C2S_R_TELEPORT,
	C2S_R_CHECKDATA,
	C2S_QUEST,	// 30
	C2S_HOLE,
	C2S_RANK,
	C2S_SETBIND,
	C2S_ETC,
	C2S_SHORTCUT,
	C2S_CRAFT,
	C2S_GUILD,
	C2S_NXSHOP,
	C2S_SCRAMBLE,
	C2S_CLIENT_LOG,	// 40
	C2S_CHAOS,
	C2S_MINIGAME,

	C2S_END
};

#pragma pack(push, 1)

struct PACKET
{
	WORD wSize;
	BYTE byType;
	char data[0];
};

#pragma pack(pop)



