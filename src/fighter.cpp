#include "fighter.h"

// TODO(onlinecop): Make these values user-configurable in settings somewhere.

constexpr int CRITICAL_HEALTH_WARNING = 5;
constexpr int CRITICAL_MAGIC_WARNING = 5;

KFighter::KFighter()
    : fighterHealthCriticalWarning(CRITICAL_HEALTH_WARNING)
    , fighterMagicCriticalWarning(CRITICAL_MAGIC_WARNING)
    , fighterName()
    , fighterExperience()
    , fighterMoney()
    , fighterLevel()
    , fighterImageDatafileX()
    , fighterImageDatafileY()
    , fighterImageDatafileWidth()
    , fighterImageDatafileHeight()
    , fighterHealth()
    , fighterMaxHealth()
    , fighterMagic()
    , fighterMaxMagic()
    , fighterDefeatItemProbability()
    , fighterDefeatItemCommon()
    , fighterDefeatItemRare()
    , fighterStealItemCommon()
    , fighterStealItemRare()
    , fighterSpriteFacing()
    , fighterAttackSpriteFrame()
    , fighterCanCriticalHit()
    , fighterWillDefend()
    , csmem()
    , ctmem()
    , current_weapon_type()
    , welem()
    , unl()
    , aux()
    , bonus()
    , bstat()
    , mrp()
    , imb_s()
    , imb_a()
    , img(nullptr)
{
    for (size_t i = 0; i < NUM_STATS; i++)
    {
        fighterStats[i] = 0;
    }
    for (size_t i = 0; i < NUM_RES; i++)
    {
        fighterResistance[i] = 0;
    }
    for (size_t i = 0; i < NUM_SPELL_TYPES; i++)
    {
        fighterSpellEffectStats[i] = 0;
    }
    for (size_t i = 0; i < 8; i++)
    {
        fighterCombatSkill[i] = 0;
    }
    for (size_t i = 0; i < 8; i++)
    {
        aip[i] = 0;
    }
    for (size_t i = 0; i < 8; i++)
    {
        atrack[i] = 0;
    }
    for (size_t i = 0; i < 2; i++)
    {
        imb[i] = 0;
    }
}

KFighter::~KFighter()
{
}

bool KFighter::isFighterHealthCritical() const
{
    if (fighterHealthCriticalWarning > 0)
    {
        return fighterHealth < (fighterMaxHealth / fighterHealthCriticalWarning);
    }
    return true;
}

bool KFighter::isFighterMagicCritical() const
{
    if (fighterMagicCriticalWarning > 0)
    {
        return fighterMagic < (fighterMaxMagic / 5);
    }
    return true;
}

/*! Characters when they are in combat */
KFighter fighter[NUM_FIGHTERS];

/*! Temp store for adjusted stats */
KFighter tempa, tempd;
