
#include "cbase.h"
#include "holo_panel.h"
#include "gstring/cgstring_globals.h"

#include "materialsystem/imaterialvar.h"

static int s_iMaterialGlobalReferenceCount;
CMaterialReference CHoloPanel::m_Materials[ MATERIALTYPE_COUNT ];

CHoloPanel::CHoloPanel() :
	m_Angles( vec3_angle ),
	m_Origin( vec3_origin ),
	m_bTransformationDirty( false )
{
	SetIdentityMatrix( m_Transformation );

	const char *pszMats[ MATERIALTYPE_COUNT ] = {
		"engine/hologui",
		"engine/hologui_scanlines",
		"engine/hologui_vertexcolor",
		"engine/hologui_scanlines_vertexcolor",
		"engine/linear_glow",
		"engine/hologui_vgui"
	};

	const char *pszZMats[ MATERIALTYPE_COUNT ] = {
		"engine/hologui_z",
		"engine/hologui_z_scanlines",
		"engine/hologui_z_vertexcolor",
		"engine/hologui_z_scanlines_vertexcolor",
		"engine/linear_z_glow",
		"engine/hologui_z_vgui"
	};
	
	bool bIgnoreZ = g_pGstringGlobals && g_pGstringGlobals->IsSpaceMap();
	const char **pMaterialNames = bIgnoreZ ? pszMats : pszZMats;

	++s_iMaterialGlobalReferenceCount;
	for (int i = 0; i < MATERIALTYPE_COUNT; ++i)
	{
		LOAD_SHARED_MATERIAL_REFERENCE( m_Materials[ i ], materials->FindMaterial( pMaterialNames[ i ], TEXTURE_GROUP_OTHER ) );
	}
}

CHoloPanel::~CHoloPanel()
{
	--s_iMaterialGlobalReferenceCount;
	for ( int i = 0; i < MATERIALTYPE_COUNT; ++i )
	{
		m_Materials[ i ]->DecrementReferenceCount();
		if ( s_iMaterialGlobalReferenceCount == 0 )
		{
			m_Materials[ i ].Shutdown();
		}
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
