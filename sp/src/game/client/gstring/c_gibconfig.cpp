
#include "cbase.h"
#include "filesystem.h"

#include "c_gibconfig.h"
#include "c_gstring_util.h"
#include "bone_setup.h"


C_GibConfig C_GibConfig::instance;

C_GibConfig::C_GibConfig()
	: BaseClass( "gibconfig_system" )
	, m_npcConfigs( CaselessCUtlStringLessThan )
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

	KeyValues::AutoDelete pKVManifest( "" );

	if ( !pKVManifest->LoadFromFile( filesystem, "scripts/gibs/manifest.txt" ) )
	{
		Assert( 0 );
		return;
	}

	// iterate all npc class type from manifest
	for ( KeyValues *pKVNpc = pKVManifest->GetFirstTrueSubKey();
		pKVNpc;
		pKVNpc = pKVNpc->GetNextTrueSubKey() )
	{
		const char *pszClassName = pKVNpc->GetName();

		if ( !pszClassName
			|| !*pszClassName )
			continue;

		NpcConfig_t npc;

		// load models and default setup for each npc class
		for ( KeyValues *pKVModel = pKVNpc->GetFirstTrueSubKey();
			pKVModel;
			pKVModel = pKVModel->GetNextTrueSubKey() )
		{
			const char *pszModelName = pKVModel->GetName();

			if ( !pszModelName
				|| !*pszModelName )
				continue;

			ModelConfig_t model;

			// load gib files per model for single/multi/explosion gibs
			for ( KeyValues *pKVGibFile = pKVModel->GetFirstValue();
				pKVGibFile;
				pKVGibFile = pKVGibFile->GetNextValue() )
			{
				const char *pszGibType = pKVGibFile->GetName();
				const char *pszGibFile = pKVGibFile->GetString();

				if ( !pszGibType
					|| !*pszGibType )
					continue;

				if ( !pszGibFile
					|| !*pszGibFile )
					continue;

				KeyValues::AutoDelete pKVGibFileContent( "" );

				// now parse the referenced gib file
				if ( !pKVGibFileContent->LoadFromFile( filesystem, VarArgs( "scripts/gibs/%s.txt", pszGibFile ) ) )
					continue;

				RagdollConfig_t ragdoll;

				// the bone name that triggers this config via hitbox
				ragdoll.m_triggerBone = pKVGibFileContent->GetString( "trigger" );

				// load all gib models for this setup
				for ( KeyValues *pKVSingleGib = pKVGibFileContent->GetFirstTrueSubKey();
					pKVSingleGib;
					pKVSingleGib = pKVSingleGib->GetNextTrueSubKey() )
				{
					const char *pszKeyName = pKVSingleGib->GetName();

					if ( !pszKeyName
						|| !FStrEq( "gib", pszKeyName ) )
						continue;

					// copy each partial model for later
					ragdoll.m_gibs.AddToTail( pKVSingleGib->MakeCopy() );
				}

				// put it into the correct list
				if ( ragdoll.m_gibs.Count() > 0 )
				{
					if ( FStrEq( "multi", pszGibType ) )
						model.m_typeConfig[ MULTI ].AddToTail( ragdoll );
					else if ( FStrEq( "explosion", pszGibType ) )
						model.m_typeConfig[ EXPLOSION ].AddToTail( ragdoll );
					else // single
						model.m_typeConfig[ SINGLE ].AddToTail( ragdoll );
				}
			}

			if ( model.m_typeConfig[ SINGLE ].Count() > 0
				|| model.m_typeConfig[ MULTI ].Count() > 0
				|| model.m_typeConfig[ EXPLOSION ].Count() > 0 )
				npc.m_modelConfigs->Insert( pszModelName, model );
		}

		if ( npc.m_modelConfigs->Count() > 0 )
			m_npcConfigs.Insert( pszClassName, npc );
	}
}

bool C_GibConfig::GetGibsForModel( GibType_e type, const char *pszClassname, const char *pszModel,
		CStudioHdr *pHdr, const char *pszHitBone, CUtlVector< KeyValues* > &gibs )
{
	if ( !pHdr )
		return false;

	unsigned short npcLookup = m_npcConfigs.Find( pszClassname );

	if ( !m_npcConfigs.IsValidIndex( npcLookup ) )
		return false;

	const NpcConfig_t &npc = m_npcConfigs[ npcLookup ];

	unsigned short modelLookup = npc.m_modelConfigs->Find( pszModel );

	if ( !npc.m_modelConfigs->IsValidIndex( modelLookup ) )
	{
		modelLookup = npc.m_modelConfigs->Find( "default" );

		if ( !npc.m_modelConfigs->IsValidIndex( modelLookup ) )
			return false;
	}

	ModelConfig_t &model = npc.m_modelConfigs->Element( modelLookup );
	CUtlVector< RagdollConfig_t > &ragdollConfigs = model.m_typeConfig[ type ];

	if ( type == EXPLOSION )
	{
		if ( ragdollConfigs.Count() > 0 )
		{
			gibs.AddVectorToTail( ragdollConfigs[ RandomInt( 0, ragdollConfigs.Count() - 1 ) ].m_gibs );
			return true;
		}
		else
		{
			return false;
		}
	}

	CUtlVector< RagdollConfig_t* > potentialConfigs;
	int iBestWeight = MAXSTUDIOBONES;

	FOR_EACH_VEC( ragdollConfigs, i )
	{
		const char *pszTriggerBone = ragdollConfigs[ i ].m_triggerBone.Get();

		if ( !pszTriggerBone
			|| !*pszTriggerBone )
			continue;

		if ( pszHitBone && *pszHitBone )
		{
			int iDepth = BoneParentDepth( pHdr, pszHitBone, pszTriggerBone );

			if ( iDepth < 0 )
				continue;

			if ( iDepth > iBestWeight )
				continue;

			iBestWeight = iDepth;
		}

		potentialConfigs.AddToTail( &ragdollConfigs[ i ] );
	}

	if ( potentialConfigs.Count() > 0 )
	{
		RagdollConfig_t *pConfig = potentialConfigs[ RandomInt( 0, potentialConfigs.Count() - 1 ) ];

		gibs.AddVectorToTail( pConfig->m_gibs );
		return true;
	}
	else
	{
		return false;
	}
}