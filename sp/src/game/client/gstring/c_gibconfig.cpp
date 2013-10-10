
#include "cbase.h"
#include "filesystem.h"

#include "c_gibconfig.h"
#include "c_gstring_util.h"
#include "bone_setup.h"


C_GibConfig C_GibConfig::instance;

C_GibConfig::C_GibConfig()
	: BaseClass( "gibconfig_system" )
	, m_npcConfigs( CaselessCUtlStringLessThan )
	, m_ragdollConfigs( CaselessCUtlStringLessThan )
{
}

C_GibConfig::~C_GibConfig()
{
}

bool C_GibConfig::Init()
{
	return true;
}

void C_GibConfig::LevelInitPreEntity()
{
	ReloadConfig();
}

void C_GibConfig::ReloadConfig()
{
	m_npcConfigs.RemoveAll();
	m_ragdollConfigs.RemoveAll();

	KeyValues::AutoDelete pKVManifestRagdoll( "" );

	if ( !pKVManifestRagdoll->LoadFromFile( filesystem, "scripts/gibs/manifest_ragdoll.txt" ) )
	{
		Assert( 0 );
		return;
	}

	for ( KeyValues *pKVRagdoll = pKVManifestRagdoll->GetFirstTrueSubKey();
		pKVRagdoll;
		pKVRagdoll = pKVRagdoll->GetNextTrueSubKey() )
	{
		const char *pszName = pKVRagdoll->GetName();

		if ( !*pszName )
			continue;

		if ( m_ragdollConfigs.Find( pszName ) != m_ragdollConfigs.InvalidIndex() )
		{
			Assert( 0 );
			Warning( "Ignoring duplicate gib config: %s\n", pszName );
			continue;
		}

		RagdollConfig_t ragdoll;

		ragdoll.m_pData = pKVRagdoll->MakeCopy();
		ragdoll.m_pData->SetName( pszName );

		unsigned short index = m_ragdollConfigs.InsertOrReplace( pszName, ragdoll );

		// find aliases for bones
		// weapons using hulls might mostly hit bones that can't be cut
		// so we can allow splitting a spline bone when the pelvis is being shot
		for ( KeyValues *pKVAlias = pKVRagdoll->GetFirstTrueSubKey();
			pKVAlias; pKVAlias = pKVAlias->GetNextTrueSubKey() )
		{
			if ( !FStrEq( "alias", pKVAlias->GetName() ) )
				continue;

			const char *pszIn = pKVAlias->GetString( "in" );
			const char *pszOut = pKVAlias->GetString( "out" );

			if ( !*pszIn
				|| !*pszOut )
				continue;

			unsigned short aliasIndex = m_ragdollConfigs[ index ].m_aliases.Find( pszIn );

			if ( !m_ragdollConfigs[ index ].m_aliases.IsValidIndex( aliasIndex ) )
			{
				CCopyableUtlVector< CUtlString > vecStr;
				vecStr.AddToTail( pszOut );
				m_ragdollConfigs[ index ].m_aliases.Insert( pszIn, vecStr );
			}
			else
			{
				m_ragdollConfigs[ index ].m_aliases[ aliasIndex ].AddToTail( pszOut );
			}
		}
	}

	KeyValues::AutoDelete pKVManifest( "" );

	if ( !pKVManifest->LoadFromFile( filesystem, "scripts/gibs/manifest_entity.txt" ) )
	{
		Assert( 0 );
		return;
	}

	// iterate all npc class types from manifest
	for ( KeyValues *pKVNpc = pKVManifest->GetFirstTrueSubKey();
		pKVNpc;
		pKVNpc = pKVNpc->GetNextTrueSubKey() )
	{
		const char *pszClassName = pKVNpc->GetName();

		if ( !*pszClassName )
			continue;

		NpcConfig_t npc;

		// load models and default setup for each npc class
		for ( KeyValues *pKVModel = pKVNpc->GetFirstValue();
			pKVModel;
			pKVModel = pKVModel->GetNextValue() )
		{
			const char *pszModelName = pKVModel->GetName();
			const char *pszRagdollName = pKVModel->GetString();

			if ( !*pszModelName
				|| !*pszRagdollName )
				continue;

			char szFixedName[ MAX_PATH ];
			Q_strncpy( szFixedName, pszModelName, sizeof( szFixedName ) );
			Q_FixSlashes( szFixedName );

			npc.m_modelConfigs.Insert( szFixedName, pszRagdollName );
		}

		if ( npc.m_modelConfigs.Count() > 0 )
			m_npcConfigs.Insert( pszClassName, npc );
	}
}

bool C_GibConfig::GetGibsForModel( const GibbingParams_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszGibGroup )
{
	if ( !params.pHdr )
		return false;

	unsigned short npcLookup = m_npcConfigs.Find( params.pszClassName );

	if ( !m_npcConfigs.IsValidIndex( npcLookup ) )
		return false;

	const NpcConfig_t &npc = m_npcConfigs[ npcLookup ];

	char szModelNameFixed[ MAX_PATH ];
	Q_strncpy( szModelNameFixed, params.pszSourceModel, sizeof( szModelNameFixed ) );
	Q_FixSlashes( szModelNameFixed );

	unsigned short modelLookup = npc.m_modelConfigs.Find( szModelNameFixed );

	if ( !npc.m_modelConfigs.IsValidIndex( modelLookup ) )
	{
		modelLookup = npc.m_modelConfigs.Find( "default" );

		if ( !npc.m_modelConfigs.IsValidIndex( modelLookup ) )
			return false;
	}

	const char *pszRagdollConfigName = npc.m_modelConfigs[ modelLookup ];

	unsigned short ragdollLookup = m_ragdollConfigs.Find( pszRagdollConfigName );

	if ( !m_ragdollConfigs.IsValidIndex( ragdollLookup ) )
		return false;

	const RagdollConfig_t &ragdollConfig = m_ragdollConfigs[ ragdollLookup ];

	Assert( ragdollConfig.m_pData );

	if ( pszGibGroup != NULL )
	{
		*pszGibGroup = ragdollConfig.m_pData->GetName();
	}

	const char *pszIdealJointName = GetBestCutJoint( ragdollConfig, params.pHdr, params.pszHitBone );

	if ( !pszIdealJointName )
		return false;

	// set up cutting for this joint
	ragdollparams_partial_t params_trunk, params_branch;
	params_trunk.trunkBones.AddToTail( pszIdealJointName );
	params_branch.rootBone = pszIdealJointName;

	gibs.AddToTail( params_trunk );
	gibs.AddToTail( params_branch );
	return true;
}

bool C_GibConfig::GetGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone )
{
	unsigned short ragdollLookup = m_ragdollConfigs.Find( params.pszParentName );

	if ( !m_ragdollConfigs.IsValidIndex( ragdollLookup ) )
		return false;

	const char *pszIdealJointName = GetBestCutJoint( m_ragdollConfigs[ ragdollLookup ], params.pHdr,
		params.pszHitBone, params.pszRootBone );

	if ( pszSplitBone != NULL )
	{
		*pszSplitBone = pszIdealJointName;
	}

	if ( !pszIdealJointName )
		return false;

	// set up cutting for this joint
	ragdollparams_partial_t params_trunk, params_branch;
	params_trunk.trunkBones.AddToTail( pszIdealJointName );
	params_branch.rootBone = pszIdealJointName;

	gibs.AddToTail( params_trunk );
	gibs.AddToTail( params_branch );
	return true;
}

static bool EvaluateCutParent( CStudioHdr *pHdr, const char *pszChild, const char *pszParent, const char *pszRoot, int &iBestDepth )
{
	// ignore any potential bones that are parents of the current root
	if ( pszRoot != NULL
		&& BoneParentDepth( pHdr, pszRoot, pszParent ) >= 0 )
		return false;

	// figure out the distance from the hit bone to the potential cut bone
	int iDepth = BoneParentDepth( pHdr, pszChild, pszParent );

	if ( iDepth < 0 )
		return false;

	if ( iDepth > iBestDepth )
		return false;

	iBestDepth = iDepth;
	return true;
}

static bool EvaluateCutChild( CStudioHdr *pHdr, const char *pszChild, const char *pszParent, int &iBestDepth )
{
	// try to cut off a child now, flip vars
	int iDepth = BoneParentDepth( pHdr, pszParent, pszChild );

	if ( iDepth <= 0 )
		return false;

	if ( iDepth > iBestDepth )
		return false;

	iBestDepth = iDepth;
	return true;
}

const char *C_GibConfig::GetBestCutJoint( const RagdollConfig_t &config, CStudioHdr *pHdr,
	const char *pszHitBone, const char *pszRootBone )
{
	// cuts a parent
	const char *pszIdealJointName = NULL;
	int iBestWeight = MAXSTUDIOBONES;

	// or a potential child
	const char *pszIdealJointNameReverse = NULL;
	int iBestReverse = MAXSTUDIOBONES;

	// for every marked cut joint
	for ( KeyValues *pKV = config.m_pData->GetFirstValue();
		pKV; pKV = pKV->GetNextValue() )
	{
		if ( !FStrEq( "joint", pKV->GetName() ) )
			continue;

		const char *pszJointName = pKV->GetString();

		if ( !*pszJointName )
			continue;

		Assert( pszHitBone );

		if ( !*pszHitBone )
			break;

		unsigned short aliasIndex = config.m_aliases.Find( pszJointName );

		// test cutting of a parent through aliases
		if ( config.m_aliases.IsValidIndex( aliasIndex ) )
		{
			FOR_EACH_VEC( config.m_aliases[ aliasIndex ], a )
			{
				if ( EvaluateCutParent( pHdr, pszHitBone, config.m_aliases[ aliasIndex ][ a ], pszRootBone, iBestWeight ) )
				{
					pszIdealJointName = pszJointName;
				}
			}
		}

		// try cutting off a parent
		if ( EvaluateCutParent( pHdr, pszHitBone, pszJointName, pszRootBone, iBestWeight ) )
		{
			pszIdealJointName = pszJointName;
		}

		// test cutting off a child bone (reverse search)
		if ( EvaluateCutChild( pHdr, pszHitBone, pszJointName, iBestReverse ) )
		{
			pszIdealJointNameReverse = pszJointName;
		}
	}

	// if we didn't find a parent to cut, take the potential child
	if ( pszIdealJointName == NULL )
	{
		pszIdealJointName = pszIdealJointNameReverse;
	}

	return pszIdealJointName;
}