
#include "cbase.h"
#include "holo_ship_objectives.h"
#include "gstring/hologui/point_holo_objective.h"

#include "vgui_controls/Label.h"
#include "materialsystem/imaterialvar.h"

using namespace vgui;

CHoloShipObjectives::CHoloShipObjectives( ISpacecraftData *pSpacecraftData )
{
	m_pLabelHeader = new Label( this, "", "#holo_gui_objectives_header_none" );

	SetOrigin( Vector( -10.0f, 11.0f, 5.9f ) );
	SetAngles( QAngle( 0, 200, 0 ) );

	m_flScale = 0.018f;
}

CHoloShipObjectives::~CHoloShipObjectives()
{
}

void CHoloShipObjectives::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelHeader->SetFont( m_FontLarge );
	m_pLabelHeader->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelHeader->SizeToContents();

	SetBounds( 0, 0, 400, m_pLabelHeader->GetTall() );
}

void CHoloShipObjectives::Draw( IMatRenderContext *pRenderContext )
{
	BaseClass::Draw( pRenderContext );

	GetColorVar()->SetVecValue( HOLO_COLOR_HIGHLIGHT );
	GetAlphaVar()->SetFloatValue( 1.0f );

	const float flTriangleStart = -0.6f;
	const float flTriangleHeight = 0.06f;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	builder.Position3f( 0, 0, flTriangleStart );
	builder.AdvanceVertex();

	builder.Position3f( 0, 0, flTriangleStart + flTriangleHeight );
	builder.AdvanceVertex();

	builder.Position3f( 0, m_flWidth * m_flScale, flTriangleStart );
	builder.AdvanceVertex();

	builder.End();
	pMesh->Draw();
}

void CHoloShipObjectives::Think( float frametime )
{
	BaseClass::Think( frametime );

	const CUtlVector< CPointHoloObjective* > &objectives = GetHoloObjectives();
	engine->Con_NPrintf( 11, "Objective count: %i", objectives.Count() );
}
