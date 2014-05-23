
#include "cbase.h"
#include "bone_setup.h"

#include "engine/ivmodelinfo.h"
#include "vcollide_parse.h"
#include "solidsetdefaults.h"

int ConvertPhysBoneToStudioBone( C_BaseAnimating *pEntity, int iPhysBone )
{
	int iStudioBone = -1;
	vcollide_t *pCollide = modelinfo->GetVCollide( pEntity->GetModelIndex() );

	if ( pCollide != NULL
		&& iPhysBone >= 0 )
	{
		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );

		while ( !pParse->Finished() )
		{
			const char *pBlock = pParse->GetCurrentBlockName();

			// need to parse the phys solids and compare to their indices
			if ( !strcmpi( pBlock, "solid" ) )
			{
				solid_t solid;
				pParse->ParseSolid( &solid, &g_SolidSetup );

				if ( solid.index == iPhysBone )
				{
					iStudioBone = pEntity->LookupBone( solid.name );
					break;
				}
			}
			else
			{
				pParse->SkipBlock();
			}
		}

		physcollision->VPhysicsKeyParserDestroy( pParse );
	}

	return iStudioBone;
}

int BoneParentDepth( CStudioHdr *pHdr, int iBone, int iPotentialParent )
{
	if ( iBone < 0
		|| iPotentialParent < 0 )
		return -1;

	if ( iBone == iPotentialParent )
		return 0;

	int iParent = pHdr->pBone( iBone )->parent;

	if ( iParent == iPotentialParent )
		return 1;

	if ( iParent < 0 )
		return -1;

	int iRet = BoneParentDepth( pHdr, iParent, iPotentialParent );

	return ( iRet < 0 ) ? -1 : 1 + iRet;
}

int BoneParentDepth( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent )
{
	Assert( pHdr );

	int iBone = Studio_BoneIndexByName( pHdr, pszBone );
	int iParent = Studio_BoneIndexByName( pHdr, pszPotentialParent );

	return BoneParentDepth( pHdr, iBone, iParent );
}

bool BoneHasParent( CStudioHdr *pHdr, int iBone, int iPotentialParent )
{
	return BoneParentDepth( pHdr, iBone, iPotentialParent ) > 0;
}

bool BoneHasParent( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent )
{
	return BoneParentDepth( pHdr, pszBone, pszPotentialParent ) > 0;
}

extern bool SupportsCascadedShadows()
{
	static bool bInit = true;
	static bool bValue = false;

	if ( bInit )
	{
		bInit = false;
		bValue = g_pMaterialSystemHardwareConfig->SupportsShaderModel_3_0() &&
			g_pMaterialSystemHardwareConfig->NumVertexShaderConstants() >= 243;
	}

	return bValue;
}
