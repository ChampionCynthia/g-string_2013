#ifndef HOLO_SHIP_OBJECTIVES_H
#define HOLO_SHIP_OBJECTIVES_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;
class CPointHoloObjective;

class CHoloShipObjectives : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipObjectives, CHoloPanelVGUI );

public:
	CHoloShipObjectives( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData );
	~CHoloShipObjectives();

	virtual void PerformLayout();

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );
	virtual void PerformLayout3D( int width, int height, bool useVR );
	virtual void OnTick();

private:
	struct Objective
	{
		Objective() :
			m_pDescription( NULL ),
			m_bCompleted( false ),
			m_iCurrentCount( 0 ),
			m_flPosition( 1.0f ),
			m_flFadeInTimer( 0.0f ),
			m_flFadeOutTimer( 0.0f ),
			m_flCompletionTimer( 0.0f )
		{}

		CHandle< CPointHoloObjective > m_Entity;
		vgui::Label *m_pDescription;

		bool m_bCompleted;
		int m_iCurrentCount;
		float m_flPosition;
		float m_flFadeInTimer;
		float m_flFadeOutTimer;
		float m_flCompletionTimer;
	};

	void AddObjective( CPointHoloObjective *pEntity );
	void RemoveObjective( int index );
	void UpdateObjective( float frametime, int index );
	void LayoutObjectives();

	void UpdateHeader();
	void UpdateObjectDescription( Objective &objective );

	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelHeader;
	CUtlVector< Objective > m_Objectives;
	int m_iObjectiveCount;
};

#endif