
#include "cbase.h"
#include "holo_panel.h"

#include "materialsystem/imaterialvar.h"
//CMaterialReference CHoloPanel::m_MaterialWhite;


CHoloPanel::CHoloPanel() :
	m_Angles( vec3_angle ),
	m_Origin( vec3_origin ),
	m_bTransformationDirty( false )
{
	SetIdentityMatrix( m_Transformation );
	m_MaterialWhite.Init( materials->FindMaterial( "engine/hologui", TEXTURE_GROUP_OTHER ) );
}

CHoloPanel::~CHoloPanel()
{
	m_MaterialWhite.Shutdown();
}

void CHoloPanel::SetAngles( const QAngle &angles )
{
	m_Angles = angles;
	m_bTransformationDirty = true;
}

void CHoloPanel::SetOrigin( const Vector &origin )
{
	m_Origin = origin;
	m_bTransformationDirty = true;
}

void CHoloPanel::PreRenderHierarchy( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight )
{
	PreRender( pRenderContext, position, maxWidth, maxHeight );
}

void CHoloPanel::DrawHierarchy( IMatRenderContext *pRenderContext )
{
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();

	if ( m_bTransformationDirty )
	{
		UpdateTransformation();
	}

	pRenderContext->MultMatrixLocal( m_Transformation );

	float color[] = { 1, 1, 1 };
	render->SetBlend( 1 );
	render->SetColorModulation( color );

	Draw( pRenderContext );

	pRenderContext->PopMatrix();
}

void CHoloPanel::ThinkHierarchy( float frametime )
{
	Think( frametime );
}

void CHoloPanel::PreRender( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight )
{
}

void CHoloPanel::Draw( IMatRenderContext *pRenderContext )
{
}

void CHoloPanel::Think( float frametime )
{
}

IMaterialVar *CHoloPanel::GetColorVar()
{
	static unsigned int iMatVar = 0;
	return m_MaterialWhite->FindVarFast( "$color", &iMatVar );
}

IMaterialVar *CHoloPanel::GetAlphaVar()
{
	static unsigned int iMatVar = 0;
	return m_MaterialWhite->FindVarFast( "$alpha", &iMatVar );
}

const QAngle &CHoloPanel::GetAngles()
{
	return m_Angles;
}

const Vector &CHoloPanel::GetOrigin()
{
	return m_Origin;
}

void CHoloPanel::UpdateTransformation()
{
	m_bTransformationDirty = false;
	AngleMatrix( m_Angles, m_Origin, m_Transformation );
}
