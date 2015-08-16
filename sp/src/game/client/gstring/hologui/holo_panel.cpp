
#include "cbase.h"
#include "holo_panel.h"

#include "materialsystem/imaterialvar.h"

CMaterialReference CHoloPanel::m_Materials[ MATERIALTYPE_COUNT ];

CHoloPanel::CHoloPanel() :
	m_Angles( vec3_angle ),
	m_Origin( vec3_origin ),
	m_bTransformationDirty( false )
{
	SetIdentityMatrix( m_Transformation );

	LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_NORMAL ], materials->FindMaterial( "engine/hologui", TEXTURE_GROUP_OTHER ) );
	LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_NORMAL_SCANLINES ], materials->FindMaterial( "engine/hologui_scanlines", TEXTURE_GROUP_OTHER ) );
	LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_VERTEXCOLOR ], materials->FindMaterial( "engine/hologui_vertexcolor", TEXTURE_GROUP_OTHER ) );
	LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_SCANLINES_VERTEXCOLOR ], materials->FindMaterial( "engine/hologui_scanlines_vertexcolor", TEXTURE_GROUP_OTHER ) );
	//LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_VERTEXCOLOR_LINEAR ], materials->FindMaterial( "engine/hologui_vertexcolor_linear", TEXTURE_GROUP_OTHER ) );
	LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ MATERIALTYPE_GLOW ], materials->FindMaterial( "engine/linear_glow", TEXTURE_GROUP_OTHER ) );
}

CHoloPanel::~CHoloPanel()
{
	for ( int i = 0; i < MATERIALTYPE_COUNT; ++i )
	{
		m_Materials[ i ]->DecrementReferenceCount();
	}
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

void CHoloPanel::PerformLayout3DHierarchy( int width, int height, bool useVR )
{
	PerformLayout3D( width, height, useVR );
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

void CHoloPanel::PerformLayout3D( int width, int height, bool useVR )
{
}

IMaterialVar *CHoloPanel::GetColorVar( MaterialType type )
{
	Assert( type >= 0 && type < MATERIALTYPE_COUNT );
	static unsigned int iMatVar[ MATERIALTYPE_COUNT ] = { 0 };
	return m_Materials[ type ]->FindVarFast( "$color", &iMatVar[ type ] );
}

IMaterialVar *CHoloPanel::GetAlphaVar( MaterialType type )
{
	Assert( type >= 0 && type < MATERIALTYPE_COUNT );
	static unsigned int iMatVar[ MATERIALTYPE_COUNT ] = { 0 };
	return m_Materials[ type ]->FindVarFast( "$alpha", &iMatVar[ type ] );
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
