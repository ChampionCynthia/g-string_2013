#include "shaderlib/cshader.h"
class gstring_holo_gui_ps20b_Static_Index
{
private:
	int m_nTEXTURE;
#ifdef _DEBUG
	bool m_bTEXTURE;
#endif
public:
	void SetTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTEXTURE = i;
#ifdef _DEBUG
		m_bTEXTURE = true;
#endif
	}
	void SetTEXTURE( bool i )
	{
		m_nTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bTEXTURE = true;
#endif
	}
private:
	int m_nSCANLINES;
#ifdef _DEBUG
	bool m_bSCANLINES;
#endif
public:
	void SetSCANLINES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSCANLINES = i;
#ifdef _DEBUG
		m_bSCANLINES = true;
#endif
	}
	void SetSCANLINES( bool i )
	{
		m_nSCANLINES = i ? 1 : 0;
#ifdef _DEBUG
		m_bSCANLINES = true;
#endif
	}
public:
	gstring_holo_gui_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bTEXTURE = false;
#endif // _DEBUG
		m_nTEXTURE = 0;
#ifdef _DEBUG
		m_bSCANLINES = false;
#endif // _DEBUG
		m_nSCANLINES = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bTEXTURE && m_bSCANLINES;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nTEXTURE ) + ( 2 * m_nSCANLINES ) + 0;
	}
};
#define shaderStaticTest_gstring_holo_gui_ps20b psh_forgot_to_set_static_TEXTURE + psh_forgot_to_set_static_SCANLINES + 0
class gstring_holo_gui_ps20b_Dynamic_Index
{
public:
	gstring_holo_gui_ps20b_Dynamic_Index()
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
#define shaderDynamicTest_gstring_holo_gui_ps20b 0
