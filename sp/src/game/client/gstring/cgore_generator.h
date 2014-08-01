#ifndef CGORE_GENERATOR_H
#define CGORE_GENERATOR_H

#include "cbase.h"

struct GoreConfig_t;

class CGoreGenerator
{
public:
	CGoreGenerator();

	void EnableBone();
	void EnableFringe();

	IMesh *GenerateMesh( IMaterial *pMaterial, const GoreConfig_t &config );

private:
};


#endif