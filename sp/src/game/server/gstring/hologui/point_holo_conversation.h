#ifndef POINT_HOLO_CONVERSATION_H
#define POINT_HOLO_CONVERSATION_H

#include "cbase.h"

class CPointHoloConversation : public CBaseEntity
{
	DECLARE_CLASS( CPointHoloConversation, CBaseEntity );
	DECLARE_DATADESC();

public:
	CPointHoloConversation();
	virtual ~CPointHoloConversation();

	void InputStart( inputdata_t &inputdata );
	void InputInterrupt( inputdata_t &inputdata );

	virtual void Activate();
	virtual void Precache();

	virtual int ObjectCaps()
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	virtual int UpdateTransmitState();

private:
	void AdvanceConversation();
	float PlayHoloSound( const char *pszSoundName, const char *pszDisplayName );

	KeyValues *m_pConversation;
	KeyValues *m_pMessage;
	KeyValues *m_pCurrentMessage;

	string_t m_strConversationName;

	float m_flNextMessage;
	float m_flCurrentDuration;
	bool m_bCanInterrupt;
	EHANDLE m_hEmittingEntity;
//#ifdef GAME_DLL
//	void Update();
//
//	string_t m_strHealthProxyName;
//	string_t m_strPositionProxyName;
//	bool m_bEnabled;
//	EHANDLE m_hHealthProxy;
//
//	COutputEvent m_OnEnabled;
//	COutputEvent m_OnDisabled;
//#endif
};

#endif
