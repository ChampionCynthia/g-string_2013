#include "shaderlib/cshader.h"
class gstring_holo_model_ps20b_Static_Index
{
private:
	int m_nDEPTH_ONLY;
#ifdef _DEBUG
	bool m_bDEPTH_ONLY;
#endif
public:
	void SetDEPTH_ONLY( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDEPTH_ONLY = i;
#ifdef _DEBUG
		m_bDEPTH_ONLY = true;
#endif
	}
	void SetDEPTH_ONLY( bool i )
	{
		m_nDEPTH_ONLY = i ? 1 : 0;
#ifdef _DEBUG
		m_bDEPTH_ONLY = true;
#endif
	}
public:
	gstring_holo_model_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bDEPTH_ONLY = false;
#endif // _DEBUG
		m_nDEPTH_ONLY = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bDEPTH_ONLY;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDEPTH_ONLY ) + 0;
	}
};
#define shaderStaticTest_gstring_holo_model_ps20b psh_forgot_to_set_static_DEPTH_ONLY + 0
class gstring_holo_model_ps20b_Dynamic_Index
{
public:
	gstring_holo_model_ps20b_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_gstring_holo_model_ps20b 0
