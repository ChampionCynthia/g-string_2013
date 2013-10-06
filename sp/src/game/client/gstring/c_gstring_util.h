#ifndef C_GSTRING_UTIL_H
#define C_GSTRING_UTIL_H



extern int BoneParentDepth( CStudioHdr *pHdr, int iBone, int iPotentialParent );
extern int BoneParentDepth( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent );

extern bool BoneHasParent( CStudioHdr *pHdr, int iBone, int iPotentialParent );
extern bool BoneHasParent( CStudioHdr *pHdr, const char *pszBone, const char *pszPotentialParent );



#endif