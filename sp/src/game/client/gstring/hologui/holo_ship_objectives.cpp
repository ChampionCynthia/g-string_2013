
#include "cbase.h"
#include "holo_ship_objectives.h"
#include "holo_utilities.h"
#include "gstring/hologui/point_holo_objective.h"

#include "vgui_controls/Label.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "materialsystem/imaterialvar.h"

using namespace vgui;

CHoloShipObjectives::CHoloShipObjectives( ISpacecraftData *pSpacecraftData ) :
	m_iObjectiveCount( -1 )
{
	m_pLabelHeader = new Label( this, "", "" );

	SetOrigin( Vector( -10.0f, 11.0f, 5.9f ) );
	SetAngles( QAngle( 0, 200, 0 ) );

	m_flScale = 0.018f;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CHoloShipObjectives::~CHoloShipObjectives()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CHoloShipObjectives::PerformLayout()
{
	BaseClass::PerformLayout();

	SetWide( 400 );

	m_pLabelHeader->SetFont( m_FontLarge );
	m_pLabelHeader->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelHeader->SetWide( GetWide() );
}

void CHoloShipObjectives::Draw( IMatRenderContext *pRenderContext )
{
	BaseClass::Draw( pRenderContext );

	GetColorVar()->SetVecValue( HOLO_COLOR_HIGHLIGHT );
	GetAlphaVar()->SetFloatValue( 1.0f );

	const float flTriangleStart = -0.5f;
	const float flTriangleHeight = 0.03f;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
	CreateSlantedRect( pMesh, 0, flTriangleStart, m_flWidth * m_flScale, flTriangleHeight );

	//CMeshBuilder builder;
	//builder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	//builder.Position3f( 0, 0, flTriangleStart );
	//builder.AdvanceVertex();

	//builder.Position3f( 0, 0, flTriangleStart + flTriangleHeight );
	//builder.AdvanceVertex();

	//builder.Position3f( 0, m_flWidth * m_flScale, flTriangleStart );
	//builder.AdvanceVertex();

	//builder.End();
	pMesh->Draw();
}

void CHoloShipObjectives::Think( float frametime )
{
	BaseClass::Think( frametime );

	FOR_EACH_VEC( m_Objectives, i )
	{
		UpdateObjective( frametime, i );
		if ( m_Objectives[ i ].m_bCompleted && m_Objectives[ i ].m_flFadeOutTimer <= 0.0f )
		{
			RemoveObjective( i );
			--i;
		}
	}

	if ( m_iObjectiveCount != m_Objectives.Count() )
	{
		m_iObjectiveCount = m_Objectives.Count();
		UpdateHeader();
	}

	LayoutObjectives();
}

void CHoloShipObjectives::OnTick()
{
	const CUtlVector< CPointHoloObjective* > &objectives = GetHoloObjectives();
	const int iObjectiveCount = m_Objectives.Count();
	FOR_EACH_VEC( objectives, o )
	{
		CPointHoloObjective *pEntity = objectives[ o ];
		bool bFound = false;
		for ( int i = 0; i < iObjectiveCount; ++i )
		{
			if ( m_Objectives[ i ].m_Entity == pEntity )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound && pEntity->GetObjectiveState() == CPointHoloObjective::OBJECTIVESTATE_ACTIVE )
		{
			AddObjective( pEntity );
		}
	}

	FOR_EACH_VEC( m_Objectives, i )
	{
		if ( !m_Objectives[ i ].m_Entity.Get() )
		{
			RemoveObjective( i );
			--i;
		}
	}
}

void CHoloShipObjectives::AddObjective( CPointHoloObjective *pEntity )
{
	Label *pLabel = new Label( this, "", pEntity->GetDescription() );
	pLabel->SetFont( m_FontSmall );
	pLabel->MakeReadyForUse();
	pLabel->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );

	Objective objective;
	objective.m_Entity = pEntity;
	objective.m_pDescription = pLabel;
	objective.m_flFadeInTimer = 0.5f;

	UpdateObjectDescription( objective );

	m_Objectives.AddToTail( objective );
	LayoutObjectives();
}

void CHoloShipObjectives::RemoveObjective( int index )
{
	m_Objectives[ index ].m_pDescription->DeletePanel();
	m_Objectives.Remove( index );
}

void CHoloShipObjectives::UpdateObjective( float frametime, int index )
{
	Objective &objective = m_Objectives[ index ];
	CPointHoloObjective *pEntity = objective.m_Entity.Get();

	if ( pEntity )
	{
		const CPointHoloObjective::ObjectiveState state = pEntity->GetObjectiveState();
		if ( !objective.m_bCompleted && state == CPointHoloObjective::OBJECTIVESTATE_COMPLETED )
		{
			objective.m_bCompleted = true;
			objective.m_pDescription->SetFgColor( Color( HOLO_COLOR255_FRIENDLY, 255 ) );
			objective.m_flCompletionTimer = 0.5f;
			objective.m_flFadeOutTimer = 4.0f;
		}

		if ( pEntity->GetCountMax() > 0 && objective.m_iCurrentCount != pEntity->GetCountCurrent() )
		{
			objective.m_iCurrentCount = pEntity->GetCountCurrent();
			UpdateObjectDescription( objective );
		}
	}

	float scale;
	if ( objective.m_bCompleted )
	{
		scale = RemapValClamped( objective.m_flFadeOutTimer, 0.0f, 1.0f, 0.0f, 0.5f );
	}
	else
	{
		scale = cos( objective.m_flFadeInTimer * M_PI_F * 5.0f * 2.0f ) * 0.5f + 0.5f;
	}

	if ( objective.m_flCompletionTimer > 0.0f )
	{
		scale *= cos( objective.m_flCompletionTimer * M_PI_F * 4.0f * 2.0f ) * 0.5f + 0.5f;
	}

	float position;
	if ( objective.m_bCompleted )
	{
		position = MIN( 1.0f, objective.m_flFadeOutTimer );
	}
	else
	{
		position = 1.0f - objective.m_flFadeInTimer * 2.0f;
	}

	if ( objective.m_flFadeInTimer > 0.0f )
	{
		objective.m_flFadeInTimer -= frametime;
		objective.m_flFadeInTimer = MAX( 0.0f, objective.m_flFadeInTimer );
	}

	if ( objective.m_flFadeOutTimer > 0.0f )
	{
		objective.m_flFadeOutTimer -= frametime;
		objective.m_flFadeOutTimer = MAX( 0.0f, objective.m_flFadeOutTimer );
	}

	if ( objective.m_flCompletionTimer > 0.0f )
	{
		objective.m_flCompletionTimer -= frametime;
		objective.m_flCompletionTimer = MAX( 0.0f, objective.m_flCompletionTimer );
	}

	int alpha = 255.0f * scale;
	objective.m_pDescription->SetAlpha( alpha );
	objective.m_flPosition = position;
}

void CHoloShipObjectives::LayoutObjectives()
{
	const int iHeaderTall = m_pLabelHeader->GetTall();
	int iTall = iHeaderTall + 10;
	FOR_EACH_VEC( m_Objectives, i )
	{
		Objective &objective = m_Objectives[ i ];
		objective.m_pDescription->SetPos( 0, iTall );
		iTall += objective.m_pDescription->GetTall() * objective.m_flPosition;
	}
	SetTall( iTall + surface()->GetFontTall( m_FontSmall ) );
}

void CHoloShipObjectives::UpdateHeader()
{
	m_pLabelHeader->SetText( m_Objectives.Count() > 0 ?
		"#holo_gui_objectives_header" : "#holo_gui_objectives_header_none" );
}

void CHoloShipObjectives::UpdateObjectDescription( Objective &objective )
{
	CPointHoloObjective *pEntity = objective.m_Entity.Get();
	const int iMax = pEntity->GetCountMax();
	if ( iMax > 0 )
	{
		const int iCurrent = pEntity->GetCountCurrent();

		wchar_t wszDescription[256];
		const wchar_t *pDescription = g_pVGuiLocalize->Find( pEntity->GetDescription() );
		if ( !pDescription )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pEntity->GetDescription(), wszDescription, sizeof(wszDescription) );
			pDescription = wszDescription;
		}

		wchar_t wszCounts[2][16];
		wchar_t wszFormatted[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( VarArgs( "%i", iCurrent ), wszCounts[0], sizeof(wszCounts[0]) );
		g_pVGuiLocalize->ConvertANSIToUnicode( VarArgs( "%i", iMax ), wszCounts[1], sizeof(wszCounts[1]) );
		g_pVGuiLocalize->ConstructString( wszFormatted, sizeof( wszFormatted ), g_pVGuiLocalize->Find( "#holo_gui_objectives_count_format" ),
			3, pDescription, wszCounts[0], wszCounts[1] );

		objective.m_pDescription->SetText( wszFormatted );
	}

	objective.m_pDescription->SizeToContents();
	int iWide = MIN( GetWide(), objective.m_pDescription->GetWide() );
	objective.m_pDescription->SetWide( iWide );
	objective.m_pDescription->SizeToContents();
}
