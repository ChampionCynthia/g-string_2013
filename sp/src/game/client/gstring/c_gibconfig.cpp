
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

	ragdollparams_partial_t params_trunk, params_branch;
	params_trunk.trunkBones.AddToTail( pszIdealJointName );
	params_branch.branchBones.AddToTail( pszIdealJointName );

	gibs.AddToTail( params_trunk );
	gibs.AddToTail( params_branch );
	return true;
}

bool C_GibConfig::GetGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone )
{
	unsigned short ragdollLookup = m_ragdollConfigs.Find( params.pszParentName );

	if ( !m_ragdollConfigs.IsValidIndex( ragdollLookup ) )
		return false;

	const char *pszIdealJointName = GetBestCutJoint( m_ragdollConfigs[ ragdollLookup ], params.pHdr, params.pszHitBone );

	if ( pszSplitBone != NULL )
	{
		*pszSplitBone = pszIdealJointName;
	}

	if ( !pszIdealJointName )
		return false;

	ragdollparams_partial_t params_trunk, params_branch;
	params_trunk.trunkBones.AddToTail( pszIdealJointName );
	params_branch.branchBones.AddToTail( pszIdealJointName );

	gibs.AddToTail( params_trunk );
	gibs.AddToTail( params_branch );
	return true;
}

const char *C_GibConfig::GetBestCutJoint( const RagdollConfig_t &config, CStudioHdr *pHdr, const char *pszHitBone )
{
	const char *pszIdealJointName = NULL;
	int iBestWeight = MAXSTUDIOBONES;

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

		if ( config.m_aliases.IsValidIndex( aliasIndex ) )
		{
			FOR_EACH_VEC( config.m_aliases[ aliasIndex ], a )
			{
				int iDepth = BoneParentDepth( pHdr, pszHitBone, config.m_aliases[ aliasIndex ][ a ] );

				if ( iDepth < 0 )
					continue;

				if ( iDepth > iBestWeight )
					continue;

				iBestWeight = iDepth;
				pszIdealJointName = pszJointName;
			}
		}

		int iDepth = BoneParentDepth( pHdr, pszHitBone, pszJointName );

		if ( iDepth < 0 )
			continue;

		if ( iDepth > iBestWeight )
			continue;

		iBestWeight = iDepth;
		pszIdealJointName = pszJointName;
	}

	return pszIdealJointName;
}