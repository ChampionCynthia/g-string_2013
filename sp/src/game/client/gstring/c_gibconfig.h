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

struct GoreParams_t
{
	const char *pszGoreClassName;
	const char *pszBone;
	bool bAsRoot;
};

struct GoreConfig_t
{
	QAngle angOrientation;
	Vector vecOffset;
	Vector vecScale;
	float flBoneSize;
	float flBoneSizeRandom;
	float flBoneChance;
	float flBoneDecorationChance;
	float flFringeSize;
	float flFringeChance;
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
	void ReloadConfig();

	bool GetGibsForModel( const GibbingParams_t &params, CUtlVector< ragdollparams_partial_t > &gibs,
		const char **pszGibGroup, const char **pszGoreGroup, const char **pszGoreMaterial );
	bool GetGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone );
	bool GetRandomGibsForGroup( const GibbingParamsRecursive_t &params, CUtlVector< ragdollparams_partial_t > &gibs, const char **pszSplitBone );
	bool GetGoreConfig( const GoreParams_t &params, GoreConfig_t &config );

private:
	// describes all partial ragdolls involved in a gib combination
	struct RagdollConfig_t
	{
		RagdollConfig_t() : m_pData( NULL ), m_aliases( CaselessCUtlStringLessThan ) {}
		RagdollConfig_t( const RagdollConfig_t &other )
			: m_aliases( CaselessCUtlStringLessThan )
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

	// multiple models per npc class
	struct NpcConfig_t
	{
		NpcConfig_t() : m_modelConfigs( CaselessCUtlStringLessThan ),
			m_goreConfigs( CaselessCUtlStringLessThan ),
			m_goreMaterials( CaselessCUtlStringLessThan ) {}
		NpcConfig_t( const NpcConfig_t &other ) :
			m_modelConfigs( CaselessCUtlStringLessThan ),
			m_goreConfigs( CaselessCUtlStringLessThan ),
			m_goreMaterials( CaselessCUtlStringLessThan )
		{
			*this = other;
		}
		NpcConfig_t &operator=( const NpcConfig_t &other )
		{
			FOR_EACH_MAP( other.m_modelConfigs, i )
			{
				m_modelConfigs.Insert( other.m_modelConfigs.Key( i ),
					other.m_modelConfigs.Element( i ) );
			}
			FOR_EACH_MAP( other.m_goreConfigs, i )
			{
				m_goreConfigs.Insert( other.m_goreConfigs.Key( i ),
					other.m_goreConfigs.Element( i ) );
			}
			FOR_EACH_MAP( other.m_goreMaterials, i )
			{
				m_goreMaterials.Insert( other.m_goreMaterials.Key( i ),
					other.m_goreMaterials.Element( i ) );
			}

			return *this;
		}

		CUtlMap< CUtlString, CUtlString > m_modelConfigs;
		CUtlMap< CUtlString, CUtlString > m_goreConfigs;
		CUtlMap< CUtlString, CUtlString > m_goreMaterials;
	};

	struct GoreBoneConfig_t
	{
		GoreConfig_t rootConfig;
		GoreConfig_t jointConfig;
	};

	struct GoreClassConfig_t
	{
		GoreClassConfig_t() : m_boneConfigs( CaselessCUtlStringLessThan ) {}
		GoreClassConfig_t( const GoreClassConfig_t &other )
			 : m_boneConfigs( CaselessCUtlStringLessThan )
		{
			*this = other;
		}
		GoreClassConfig_t &operator=( const GoreClassConfig_t &other )
		{
			FOR_EACH_MAP( other.m_boneConfigs, i )
			{
				m_boneConfigs.Insert( other.m_boneConfigs.Key( i ),
					other.m_boneConfigs.Element( i ) );
			}

			return *this;
		}

		CUtlMap< CUtlString, GoreBoneConfig_t > m_boneConfigs;
	};

	// lookup config by class name
	CUtlMap< CUtlString, NpcConfig_t > m_npcConfigs;

	// shared partial ragdoll layouts
	CUtlMap< CUtlString, RagdollConfig_t > m_ragdollConfigs;

	CUtlMap< CUtlString, GoreClassConfig_t > m_goreConfigs;

	const char *GetBestCutJoint( const RagdollConfig_t &config, CStudioHdr *pHdr, const char *pszHitBone,
		const char *pszRootBone = NULL, CBoneBitList *pJointList = NULL );
};


#endif