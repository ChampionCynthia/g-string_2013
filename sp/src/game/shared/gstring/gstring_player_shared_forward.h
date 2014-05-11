#ifndef GSTRING_PLAYER_SHARED_FORWARD_H
#define GSTRING_PLAYER_SHARED_FORWARD_H

#ifdef CLIENT_DLL
class C_GstringPlayer;
#define CSharedPlayer C_GstringPlayer
#else
class CGstringPlayer;
#define CSharedPlayer CGstringPlayer
#endif

#endif