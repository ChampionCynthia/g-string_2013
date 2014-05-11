#ifndef SPACE_CRAFT_PROJECTILE_H
#define SPACE_CRAFT_PROJECTILE_H

class CSpacecraftProjectile : public CBaseEntity
{
	DECLARE_CLASS( CSpacecraftProjectile, CBaseEntity );
	DECLARE_NETWORKCLASS();
public:

	CSpacecraftProjectile();
	virtual ~CSpacecraftProjectile();

protected:


private:
};

#endif