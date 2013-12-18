#ifndef C_GIBCONFIG_H
#define C_GIBCONFIG_H

#include "igamesystem.h"

extern ConVar gstring_gibbing_chance;
extern ConVar gstring_gibbing_explosion_chance;
extern ConVar gstring_gibbing_explosion_recursive_chance;

inline bool CaselessCUtlStringLessThan( const CUtlString &lhs, const CUtlString &rhs )
{
	if ( !lhs ) return false;
	if ( !rhs ) return true;

	return ( Q_stricmp( lhs, rhs) < 0 );
}

namespace GibTypes
{
	enum GibType_e
	{
		SINGLE = 0,
		MULTI,
		EXPLOSION,

		COUNT,
	};
}
typedef GibTypes::GibType_e GibTypeId;

struct GibbingParams_t
{
	GibTypeId type;

	const char *pszClassName;
	const char *pszSourceModel;
	const char *pszHitBone;
	CStudioHdr *pHdr;
};

struct GibbingParamsRecursive_t
{
	const char *pszParentName;
	int iGibIndex;
	const char *pszHitBone;
	const char *pszRootBone;
	CStudioHdr *pHdr;
	CBoneBitList *pJointBones;
};

class C_GibConfig : public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;

	static C_GibConfig instance;

	C_GibConfig();
	~C_GibConfig();
public:

	static C_GibConfig *GetInstance() { return &instance; }

	virtual bool Init();

	virtual void LevelInitPreEntity();

	bool GetGibsForModel( const GibbingParams_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszGibGroup );
	bool GetGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone );
	bool GetRandomGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone );

private:

	void ReloadConfig();

	// describes all partial ragdolls involved in a gib combination
	struct RagdollConfig_t
	{
		RagdollConfig_t() : m_pData( NULL ), m_aliases( CaselessCUtlStringLessThan ) {}
		RagdollConfig_t( const RagdollConfig_t &other )
		{
			*this = other;
		}
		~RagdollConfig_t()
		{
			if ( m_pData != NULL )
				m_pData->deleteThis();
		}
		RagdollConfig_t &operator=( const RagdollConfig_t &other )
		{
			m_aliases.SetLessFunc( CaselessCUtlStringLessThan );
			m_pData = NULL;

			if ( other.m_pData != NULL )
				m_pData = other.m_pData->MakeCopy();

			FOR_EACH_MAP( other.m_aliases, i )
			{
				CCopyableUtlVector< CUtlString > strVec;
				FOR_EACH_VEC( other.m_aliases[ i ], v )
					strVec.AddToTail( other.m_aliases[ i ][ v ] );

				if ( strVec.Count() > 0 )
					m_aliases.Insert( other.m_aliases.Key( i ), strVec );
			}

			return *this;
		}

		KeyValues *m_pData;

		CUtlMap< CUtlString, CCopyableUtlVector< CUtlString > > m_aliases;
	};

	// different gib setups per model
	//struct ModelConfig_t
	//{
	//	CUtlString m_strConfig;
	//};

	// multiple models per npc class
	struct NpcConfig_t
	{
		NpcConfig_t() : m_modelConfigs( CaselessCUtlStringLessThan ) {}
		NpcConfig_t( const NpcConfig_t &other )
		{
			*this = other;
		}
		NpcConfig_t &operator=( const NpcConfig_t &other )
		{
			m_modelConfigs.SetLessFunc( CaselessCUtlStringLessThan );
			FOR_EACH_MAP( other.m_modelConfigs, i )
			{
				m_modelConfigs.Insert( other.m_modelConfigs.Key( i ),
					other.m_modelConfigs.Element( i ) );
			}

			return *this;
		}

		CUtlMap< CUtlString, CUtlString > m_modelConfigs;
	};

	// lookup config by class name
	CUtlMap< CUtlString, NpcConfig_t > m_npcConfigs;

	// shared partial ragdoll layouts
	CUtlMap< CUtlString, RagdollConfig_t > m_ragdollConfigs;

	const char *GetBestCutJoint( const RagdollConfig_t &config, CStudioHdr *pHdr, const char *pszHitBone, const char *pszRootBone = NULL );
};


#endif