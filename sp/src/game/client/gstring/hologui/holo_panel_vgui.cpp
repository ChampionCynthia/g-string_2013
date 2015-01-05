
#include "cbase.h"
#include "holo_panel_vgui.h"

#include "vgui_controls/Controls.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "view_shared.h"

using namespace vgui;

CHoloPanelVGUI::CHoloPanelVGUI() :
	m_FontLarge( 0 ),
	m_FontSmall( 0 ),
	m_FontSmallMono( 0 ),
	m_vecPanelWorldOffset( vec2_origin ),
	m_flWidth( 1.0f ),
	m_flHeight( 1.0f ),
	m_flScale( 0.022f )
{
	m_vecUVs[ 0 ].Init();
	m_vecUVs[ 1 ].Init();
	m_MaterialVGUI.Init( materials->FindMaterial( "engine/hologui_vgui", TEXTURE_GROUP_OTHER ) );
}

void CHoloPanelVGUI::Setup()
{
	InvalidateLayout( true, true );
	MakeReadyForUse();
}

void CHoloPanelVGUI::Paint()
{
#if 0
	surface()->DrawSetColor( Color( 255, 255, 255, 6 ) );
	surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
#endif
}

void CHoloPanelVGUI::PerformLayout()
{
	Panel::PerformLayout();

	//SetBounds( 0, 0, 1024, 1024 );
	SetPaintBackgroundEnabled( false );
}

void CHoloPanelVGUI::ApplySchemeSettings( IScheme *pScheme )
{
	Panel::ApplySchemeSettings( pScheme );

	m_FontLarge = pScheme->GetFont( "HoloGUILarge", false );
	m_FontSmall = pScheme->GetFont( "HoloGUISmall", false );
	m_FontSmallMono = pScheme->GetFont( "HoloGUISmallMonoSpaced", false );
}

void CHoloPanelVGUI::PreRender( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight )
{
	int desiredWidth = GetWide() + 2;
	int desiredHeight = GetTall() + 2;

	Assert( desiredWidth > 0 && desiredWidth <= maxWidth );
	Assert( desiredHeight > 0 && desiredHeight <= maxHeight );

	if ( maxWidth - ( position.x + position.width ) >= desiredWidth )
	{
		position.x += position.width;
	}
	else
	{
		position.x = 0;
		position.y += position.height;
	}

	position.width = desiredWidth;
	position.height = MAX( position.height, desiredHeight );

	desiredWidth -= 2;
	desiredHeight -= 2;
	++position.x;
	++position.y;

	CViewSetup setup;
	setup.x = position.x;
	setup.y = position.y;
	setup.width = desiredWidth;
	setup.height = desiredHeight;
	setup.m_bOrtho = true;
	setup.m_flAspectRatio = 1;
	setup.fov = 90;
	setup.zFar = 9999;
	setup.zNear = 10;

	m_vecUVs[ 0 ].x = position.x / (float)maxWidth;
	m_vecUVs[ 0 ].y = position.y / (float)maxHeight;
	m_vecUVs[ 1 ].x = ( position.x + desiredWidth ) / (float)maxWidth;
	m_vecUVs[ 1 ].y = ( position.y + desiredHeight ) / (float)maxHeight;
	m_flWidth = desiredWidth;
	m_flHeight = desiredHeight;

	Frustum frustum;
	render->Push2DView( setup, 0, pRenderContext->GetRenderTarget(), frustum );

	surface()->DrawSetAlphaMultiplier( 1 );

	surface()->PushMakeCurrent( GetVPanel(), false );
	ipanel()->PaintTraverse( GetVPanel(), true );
	surface()->PopMakeCurrent( GetVPanel() );

	surface()->SwapBuffers( GetVPanel() );

	render->PopView( frustum );
}

void CHoloPanelVGUI::Draw( IMatRenderContext *pRenderContext )
{
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, m_MaterialVGUI );
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	builder.Position3f( 0, m_vecPanelWorldOffset.x, m_vecPanelWorldOffset.y );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 0 ].x, m_vecUVs[ 0 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_flWidth * m_flScale + m_vecPanelWorldOffset.x, m_vecPanelWorldOffset.y );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 1 ].x, m_vecUVs[ 0 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_flWidth * m_flScale + m_vecPanelWorldOffset.x, -m_flHeight * m_flScale + m_vecPanelWorldOffset.y );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 1 ].x, m_vecUVs[ 1 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_vecPanelWorldOffset.x, -m_flHeight * m_flScale + m_vecPanelWorldOffset.y );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 0 ].x, m_vecUVs[ 1 ].y );
	builder.AdvanceVertex();

	builder.End();
	pMesh->Draw();


#if 0
	const float debugScale = 4;
	pRenderContext->PushMatrix();
	matrix3x4_t debugRotation;
	MatrixBuildRotationAboutAxis( Vector( 0, 0, 1 ), 180.0f, debugRotation );
	pRenderContext->LoadMatrix( debugRotation );
	pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial( MATERIALTYPE_VERTEXCOLOR ) );
	builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	builder.Position3f( 0, m_vecUVs[ 0 ].x * debugScale, m_vecUVs[ 0 ].y * -debugScale );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 0 ].x, m_vecUVs[ 0 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_vecUVs[ 1 ].x * debugScale, m_vecUVs[ 0 ].y * -debugScale );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 1 ].x, m_vecUVs[ 0 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_vecUVs[ 1 ].x * debugScale, m_vecUVs[ 1 ].y * -debugScale );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 1 ].x, m_vecUVs[ 1 ].y );
	builder.AdvanceVertex();
	builder.Position3f( 0, m_vecUVs[ 0 ].x * debugScale, m_vecUVs[ 1 ].y * -debugScale );
	builder.Color4f( 1, 1, 1, 1 );
	builder.TexCoord2f( 0, m_vecUVs[ 0 ].x, m_vecUVs[ 1 ].y );
	builder.AdvanceVertex();

	builder.End();
	pMesh->Draw();
	pRenderContext->PopMatrix();
#endif
}

void CHoloPanelVGUI::Think( float frametime )
{
	ipanel()->Think( GetVPanel() );
}

