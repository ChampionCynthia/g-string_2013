
#include "cbase.h"
#include "functionproxy.h"
#include "cspacecraft.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGstringProxySpacecraftEngine : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CGstringProxySpacecraftEngine::OnBind( void *pC_BaseEntity )
{
	if ( !pC_BaseEntity )
		return;

	CSpacecraft *pSpacecraft = assert_cast< CSpacecraft* >( BindArgToEntity( pC_BaseEntity ) );

	SetFloatResult( pSpacecraft->GetEngineAlpha() );
}

EXPOSE_INTERFACE( CGstringProxySpacecraftEngine, IMaterialProxy, "GStringSpacecraftEngine" IMATERIAL_PROXY_INTERFACE_VERSION );
