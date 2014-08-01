
#include "cbase.h"
#include "filesystem.h"

#include "c_gibconfig.h"
#include "c_clientpartialragdoll.h"
#include "c_gstring_util.h"
#include "bone_setup.h"

ConVar gstring_gibbing_chance( "gstring_gibbing_chance", "20", FCVAR_CHEAT, "Probably of gibbing by normal impacts." );
ConVar gstring_gibbing_explosion_chance( "gstring_gibbing_explosion_chance", "90", FCVAR_CHEAT, "Probably of gibbing by explosion impacts." );
ConVar gstring_gibbing_explosion_recursive_chance( "gstring_gibbing_explosion_recursive_chance", "70", FCVAR_CHEAT, "Probably of gibbing multiple times by explosion impacts." );

CON_COMMAND( gstring_gibbing_reload_config, "" )
{
	C_GibConfig::GetInstance()->ReloadConfig();

	for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
	{
		C_ClientPartialRagdoll *pPartialRagdoll = dynamic_cast< C_ClientPartialRagdoll* >( pEnt );
		if ( pPartialRagdoll != NULL )
		{
			pPartialRagdoll->DestroyGore();
			pPartialRagdoll->RebuildGore();
		}
	}
}

static void ParseGoreConfig( KeyValues *pKV, GoreConfig_t &config )
{
	if ( pKV == NULL )
	{
		config.angOrientation.Init();
		config.vecOffset.Init();
		config.vecScale.Init( 1.0f, 1.0f, 1.0f );
		config.flBoneSize = 0.0f;
		config.flBoneSizeRandom = 0.0f;
		config.flBoneChance = 100.0f;
		config.flBoneDecorationChance = 0.0f;
		config.flFringeSize = 0.0f;
		config.flFringeChance = 100.0f;
	}
	else
	{
		UTIL_StringToVector( config.angOrientation.Base(), pKV->GetString( "orientation", "0 0 0" ) );
		UTIL_StringToVector( config.vecOffset.Base(), pKV->GetString( "offset", "0 0 0" ) );
		UTIL_StringToVector( config.vecScale.Base(), pKV->GetString( "scale", "1 1 1" ) );
		config.flBoneSize = pKV->GetFloat( "bone_size" );
		config.flBoneSizeRandom = pKV->GetFloat( "bone_size_random" );
		config.flBoneChance = pKV->GetFloat( "bone_chance", 100.0f );
		config.flBoneDecorationChance = pKV->GetFloat( "bone_decoration_chance", 0.0f );
		config.flFringeSize = pKV->GetFloat( "fringe_size" );
		config.flFringeChance = pKV->GetFloat( "fringe_chance", 100.0f );
	}
}

C_GibConfig C_GibConfig::instance;

C_GibConfig::C_GibConfig()
	: BaseClass( "gibconfig_system" )
	, m_npcConfigs( CaselessCUtlStringLessThan )
	, m_ragdollConfigs( CaselessCUtlStringLessThan )
	, m_goreConfigs( CaselessCUtlStringLessThan )
{
}

C_GibConfig::~C_GibConfig()
{
}

bool C_GibConfig::Init()
{
	ReloadConfig();
	return true;
}

void C_GibConfig::ReloadConfig()
{
	m_npcConfigs.RemoveAll();
	m_ragdollConfigs.RemoveAll();
	m_goreConfigs.RemoveAll();

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
		for ( KeyValues *pKVModel = pKVNpc->GetFirstTrueSubKey();
			pKVModel;
			pKVModel = pKVModel->GetNextTrueSubKey() )
		{
			const char *pszModelName = pKVModel->GetName();
			const char *pszRagdollName = pKVModel->GetString( "ragdoll" );

			if ( !*pszModelName
				|| !*pszRagdollName )
				continue;

			char szFixedName[ MAX_PATH ];
			Q_strncpy( szFixedName, pszModelName, sizeof( szFixedName ) );
			Q_FixSlashes( szFixedName );

			npc.m_modelConfigs.Insert( szFixedName, pszRagdollName );

			const char *pszGoreName = pKVModel->GetString( "gore" );
			if ( *pszGoreName )
			{
				npc.m_goreConfigs.Insert( szFixedName, pszGoreName );
			}

			const char *pszGoreMaterialName = pKVModel->GetString( "gore_material" );
			if ( *pszGoreMaterialName )
			{
				npc.m_goreMaterials.Insert( szFixedName, pszGoreMaterialName );
			}
		}

		if ( npc.m_modelConfigs.Count() > 0 )
			m_npcConfigs.Insert( pszClassName, npc );
	}

	KeyValues::AutoDelete pKVManifestGore( "" );

	if ( !pKVManifestGore->LoadFromFile( filesystem, "scripts/gibs/manifest_gore.txt" ) )
	{
		Assert( 0 );
		return;
	}

	for ( KeyValues *pKVGore = pKVManifestGore->GetFirstTrueSubKey();
		pKVGore;
		pKVGore = pKVGore->GetNextTrueSubKey() )
	{
		const char *pszName = pKVGore->GetName();

		if ( !*pszName )
			continue;

		GoreClassConfig_t classConfig;

		for ( KeyValues *pKVBone = pKVGore->GetFirstTrueSubKey();
			pKVBone;
			pKVBone = pKVBone->GetNextTrueSubKey() )
		{
			const char *pszBoneName = pKVBone->GetName();

			if ( !*pszBoneName )
				continue;

			GoreBoneConfig_t config;
			ParseGoreConfig( pKVBone->FindKey( "root" ), config.rootConfig );
			ParseGoreConfig( pKVBone->FindKey( "joint" ), config.jointConfig );
			classConfig.m_boneConfigs.Insert( pszBoneName, config );
		}

		m_goreConfigs.Insert( pszName, classConfig );
	}
}

bool C_GibConfig::GetGibsForModel( const GibbingParams_t &params, CUtlVector< ragdollparams_partial_t > &gibs,
	const char **pszGibGroup, const char **pszGoreGroup, const char **pszGoreMaterial )
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
	const unsigned short ragdollLookup = m_ragdollConfigs.Find( pszRagdollConfigName );

	if ( !m_ragdollConfigs.IsValidIndex( ragdollLookup ) )
		return false;

	const RagdollConfig_t &ragdollConfig = m_ragdollConfigs[ ragdollLookup ];
	Assert( ragdollConfig.m_pData );

	*pszGibGroup = ragdollConfig.m_pData->GetName();

	unsigned short goreNameLookup = npc.m_goreConfigs.Find( szModelNameFixed );
	if ( !npc.m_goreConfigs.IsValidIndex( goreNameLookup ) )
	{
		goreNameLookup = npc.m_goreConfigs.Find( "default" );
	}
	if ( npc.m_goreConfigs.IsValidIndex( goreNameLookup ) )
	{
		*pszGoreGroup = npc.m_goreConfigs[ goreNameLookup ];
	}
	else
	{
		*pszGoreGroup = NULL;
	}

	unsigned short goreMaterialNameLookup = npc.m_goreMaterials.Find( szModelNameFixed );
	if ( !npc.m_goreMaterials.IsValidIndex( goreMaterialNameLookup ) )
	{
		goreMaterialNameLookup = npc.m_goreMaterials.Find( "default" );
	}
	if ( npc.m_goreMaterials.IsValidIndex( goreMaterialNameLookup ) )
	{
		*pszGoreMaterial = npc.m_goreMaterials[ goreMaterialNameLookup ];
	}
	else
	{
		*pszGoreMaterial = NULL;
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
	if ( params.pszHitBone == NULL )
		return false;

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

bool C_GibConfig::GetRandomGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone )
{
	const unsigned short ragdollLookup = m_ragdollConfigs.Find( params.pszParentName );

	if ( !m_ragdollConfigs.IsValidIndex( ragdollLookup ) )
		return false;

	const char *pszIdealJointName = NULL;

	for ( int i = 0; i < params.pHdr->numbones(); i++ )
	{
		const char *pszBone = params.pHdr->pBone( i )->pszName();

		if ( params.pszRootBone != NULL
			&& BoneParentDepth( params.pHdr, pszBone, params.pszRootBone ) < 1 )
			continue;

		const char *pszPotentialJoint = GetBestCutJoint( m_ragdollConfigs[ ragdollLookup ], params.pHdr,
			pszBone, params.pszRootBone );

		if ( pszPotentialJoint != NULL )
		{
			// this joint is cut already
			if ( params.pJointBones->IsBitSet( Studio_BoneIndexByName( params.pHdr, pszPotentialJoint ) ) )
				continue;

			// wrong parent
			if ( params.pszRootBone != NULL
				&& BoneParentDepth( params.pHdr, pszPotentialJoint, params.pszRootBone ) < 1 )
				continue;

			bool bJointIsParent = false;

			for ( int j = 0; j < params.pJointBones->GetNumBits(); j++ )
			{
				int index = params.pJointBones->Get( j );

				if ( index <= 0 || index >= params.pHdr->numbones() )
					continue;

				const char *pszJointBone = params.pHdr->pBone( index )->pszName();

				if ( BoneParentDepth( params.pHdr, pszPotentialJoint,
					pszJointBone ) > 0 )
				{
					bJointIsParent = true;
					break;
				}
			}

			// is child of cut joint so no.
			if ( bJointIsParent )
				continue;

			pszIdealJointName = pszPotentialJoint;
			break;
		}
	}

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

bool C_GibConfig::GetGoreConfig( const GoreParams_t &params, GoreConfig_t &config )
{
	const unsigned int goreClassLookup = m_goreConfigs.Find( params.pszGoreClassName );
	if ( !m_goreConfigs.IsValidIndex( goreClassLookup ) )
	{
		return false;
	}

	const GoreClassConfig_t &goreClassConfig = m_goreConfigs[ goreClassLookup ];
	const unsigned short boneLookup = goreClassConfig.m_boneConfigs.Find( params.pszBone );
	if ( !goreClassConfig.m_boneConfigs.IsValidIndex( boneLookup ) )
	{
		return false;
	}

	const GoreBoneConfig_t &boneConfig = goreClassConfig.m_boneConfigs[ boneLookup ];
	if ( params.bAsRoot )
	{
		config = boneConfig.rootConfig;
	}
	else
	{
		config = boneConfig.jointConfig;
	}
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