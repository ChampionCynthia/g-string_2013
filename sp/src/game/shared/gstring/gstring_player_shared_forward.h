#ifndef GSTRING_PLAYER_SHARED_FORWARD_H
#define GSTRING_PLAYER_SHARED_FORWARD_H

#ifdef CLIENT_DLL
class C_GstringPlayer;
#define CSharedPlayer C_GstringPlayer
#define CSharedEntity C_BaseEntity
#else
class CGstringPlayer;
#define CSharedPlayer CGstringPlayer
#define CSharedEntity CBaseEntity
#endif

#endif