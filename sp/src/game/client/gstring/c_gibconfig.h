#ifndef C_GIBCONFIG_H
#define C_GIBCONFIG_H

#include "igamesystem.h"

inline bool CaselessCUtlStringLessThan( const CUtlString &lhs, const CUtlString &rhs )
{
	if ( !lhs ) return false;
	if ( !rhs ) return true;

	return ( Q_stricmp( lhs, rhs) < 0 );
}

class C_GibConfig : public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;

	static C_GibConfig instance;

	C_GibConfig();
	~C_GibConfig();
public:

	enum GibType_e
	{
		SINGLE = 0,
		MULTI,
		EXPLOSION,

		COUNT,
	};

	static C_GibConfig *GetInstance() { return &instance; }

	virtual bool Init();

	virtual void LevelInitPreEntity();

	bool GetGibsForModel( GibType_e type, const char *pszClassname, const char *pszModel,
		CStudioHdr *pHdr, const char *pszHitBone, CUtlVector< KeyValues* > &gibs );

private:

	void ReloadConfig();

	// describes all partial ragdolls involved in a gib combination
	struct RagdollConfig_t
	{
		RagdollConfig_t() {}
		RagdollConfig_t( const RagdollConfig_t &other )
		{
			*this = other;
		}
		~RagdollConfig_t()
		{
			FOR_EACH_VEC( m_gibs, i )
				m_gibs[ i ]->deleteThis();
		}
		RagdollConfig_t &operator=( const RagdollConfig_t &other )
		{
			m_triggerBone = other.m_triggerBone;

			FOR_EACH_VEC( other.m_gibs, i )
				m_gibs.AddToTail( other.m_gibs[ i ]->MakeCopy() );

			return *this;
		}

		// use this config if we hit this hitbox or one of its children
		CUtlString m_triggerBone;

		CUtlVector< KeyValues* > m_gibs;
	};

	// different gib setups per model
	struct ModelConfig_t
	{
		ModelConfig_t() {}
		ModelConfig_t( const ModelConfig_t &other )
		{
			*this = other;
		}
		ModelConfig_t &operator=( const ModelConfig_t &other )
		{
			for ( int t = 0; t < COUNT; t++ )
				FOR_EACH_VEC( other.m_typeConfig[ t ], i )
					m_typeConfig[ t ].AddToTail( RagdollConfig_t( other.m_typeConfig[ t ][ i ] ) );
			return *this;
		}

		CUtlVector< RagdollConfig_t > m_typeConfig[ COUNT ];
	};

	// multiple models per npc class
	struct NpcConfig_t
	{
		NpcConfig_t()
		{
			m_modelConfigs = new CUtlMap< CUtlString, ModelConfig_t >( CaselessCUtlStringLessThan );
		}
		NpcConfig_t( const NpcConfig_t &other )
		{
			*this = other;
		}
		~NpcConfig_t()
		{
			delete m_modelConfigs;
		}
		NpcConfig_t &operator=( const NpcConfig_t &other )
		{
			m_modelConfigs = new CUtlMap< CUtlString, ModelConfig_t >( CaselessCUtlStringLessThan );

			FOR_EACH_MAP( *other.m_modelConfigs, i )
			{
				m_modelConfigs->Insert( other.m_modelConfigs->Key( i ),
					other.m_modelConfigs->Element( i ) );
			}

			return *this;
		}

		CUtlMap< CUtlString, ModelConfig_t > *m_modelConfigs;
	};

	// lookup config by class name
	CUtlMap< CUtlString, NpcConfig_t > m_npcConfigs;

	// shared partial ragdoll layouts
	//CUtlMap< CUtlString, RagdollConfig_t > m_ragdollConfigs;
};


#endif