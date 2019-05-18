#include "combat.h"

#include "constants.h"
#include "draw.h"
#include "effects.h"
#include "enemyc.h"
#include "enums.h"
#include "gfx.h"
#include "heroc.h"
#include "imgcache.h"
#include "input.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "masmenu.h"
#include "menu.h"
#include "music.h"
#include "platform.h"
#include "random.h"
#include "res.h"
#include "setup.h"
#include "structs.h"
#include "timing.h"

#include <cstdio>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

KCombat kqCombat;


KCombat::KCombat()
    : combatend(ECombatEnd::NOT_IN_COMBAT)
    , fighterImageDatafileX()
    , fighterImageDatafileY()
    , num_enemies()
    , rcount()
    , vspell()
    , chanceEnemiesSurpriseYou()
    , backart(nullptr)
    , curw()
    , changeYouSurpriseEnemies()
{
    for (size_t i = 0; i < NUM_FIGHTERS; ++i)
    {
        bIsEtherEffectActive[i] = false;
        ta[i] = 0;
        deffect[i] = 0;
        nspeed[i] = 0;
        bspeed[i] = 0;
    }
}

KCombat::~KCombat()
{
}

KCombat::eAttackResult KCombat::attack_result(size_t fighterIndexAttacker, size_t fighterIndexDefender)
{
    if (fighterIndexAttacker >= NUM_FIGHTERS)
    {
        return eAttackResult::ATTACK_MISS;
    }
    if (fighterIndexDefender >= NUM_FIGHTERS)
    {
        return eAttackResult::ATTACK_MISS;
    }

    int check_for_critical_hit;
    int attacker_critical_status = 0;
    int crit_hit = 0;
    int base;
    int to_hit;
    int mult;
    int dmg; /* extra */
    int attacker_attack;
    int attacker_hit;
    size_t attacker_weapon_element;
    int defender_defense;
    int defender_evade;

    attacker_attack = tempa.fighterStats[A_ATT];
    attacker_hit = tempa.fighterStats[A_HIT];
    attacker_weapon_element = tempa.welem;
    defender_defense = tempd.fighterStats[A_DEF];
    defender_evade = tempd.fighterStats[A_EVD];

    /*  JB: check to see if the attacker is in critical status...  */
    /*      increases chance for a critical hit                    */
    if (tempa.fighterMaxHealth > 250)
    {
        if (tempa.fighterHealth <= 50)
        {
            attacker_critical_status = 1;
        }
    }
    else
    {
        if (tempa.fighterHealth <= (tempa.fighterMaxHealth / 5))
        {
            attacker_critical_status = 1;
        }
    }

    /*  JB: check to see if the defender is 'defending'  */
    if (tempd.fighterWillDefend == 1)
    {
        defender_defense = (defender_defense * 3) / 2;
    }

    /*  JB: if the attacker is empowered by trueshot  */
    if (tempa.fighterSpellEffectStats[S_TRUESHOT] > 0)
    {
        fighter[fighterIndexAttacker].fighterSpellEffectStats[S_TRUESHOT] = 0;
        defender_evade = 0;
    }

    attacker_attack += (tempa.fighterStats[tempa.bstat] * tempa.bonus / 100);
    if (attacker_attack < DMG_RND_MIN * 5)
    {
        base = kqrandom->random_range_exclusive(0, DMG_RND_MIN);
    }
    else
    {
        base = kqrandom->random_range_exclusive(0, attacker_attack / 5);
    }

    base += attacker_attack - defender_defense;
    if (base < 1)
    {
        base = 1;
    }

    mult = 0;
    to_hit = attacker_hit + defender_evade;
    if (to_hit < 1)
    {
        to_hit = 1;
    }

    if (kqrandom->random_range_exclusive(0, to_hit) < attacker_hit)
    {
        mult++;
    }

    /*  JB: If the defender is etherealized, set mult to 0  */
    if (tempd.fighterSpellEffectStats[S_ETHER] > 0)
    {
        mult = 0;
    }

    if (mult > 0)
    {
        if (tempd.fighterCanCriticalHit == 1)
        {
            check_for_critical_hit = 1;
            if (attacker_critical_status == 1)
            {
                check_for_critical_hit = 2;
            }

            /* PH I _think_ this makes Sensar 2* as likely to make a critical hit */
            if (pidx[fighterIndexAttacker] == SENSAR)
            {
                check_for_critical_hit = check_for_critical_hit * 2;
            }

            check_for_critical_hit = (20 - check_for_critical_hit);
            if (kqrandom->random_range_exclusive(0, 20) >= check_for_critical_hit)
            {
                crit_hit = 1;
                base = ((int)base * 3) / 2;
            }
        }

        /*  JB: If affected by a NAUSEA/MALISON spell, the defender takes more damage than normal  */
        if (tempd.fighterSpellEffectStats[S_MALISON] > 0)
        {
            base *= (int)5 / 4;
        }

        /*  JB: check for elemental/status weapons  */
        if (base < 1)
        {
            base = 1;
        }

        // Depending on what element the attacker had on the weapon (or its main attack),
        // the defender may take damage, get an effect set on them (sleep, blind, etc.).
        eResistance defenderRes = static_cast<eResistance>(attacker_weapon_element - 1);
        switch (defenderRes)
        {
        case R_EARTH:
        case R_BLACK:
        case R_FIRE:
        case R_THUNDER:
        case R_AIR:
        case R_WHITE:
        case R_WATER:
        case R_ICE:
        {
            base = res_adjust(fighterIndexDefender, defenderRes, base);
            break;
        }
        case R_POISON:
        case R_BLIND:
        case R_CHARM:
        case R_PARALYZE:
        case R_PETRIFY:
        case R_SILENCE:
        case R_SLEEP:
        {
            uint8_t spellEffectStatIndex = defenderRes - R_POISON;
            if ((res_throw(fighterIndexDefender, defenderRes) == 0) && (fighter[fighterIndexDefender].fighterSpellEffectStats[spellEffectStatIndex] == 0))
            {
                if (non_dmg_save(fighterIndexDefender, 50) == 0)
                {
                    if ((defenderRes == R_POISON) || (defenderRes == R_PETRIFY) || (defenderRes == R_SILENCE))
                    {
                        tempd.fighterSpellEffectStats[spellEffectStatIndex] = 1;
                    }
                    else
                    {
                        tempd.fighterSpellEffectStats[spellEffectStatIndex] = kqrandom->random_range_exclusive(2, 5);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    /*  JB: Apply the damage multiplier  */
    /*  RB FIXME: check if the changes I made here didn't break something  */
    /* TT TODO:
     * If magic, attacks, etc. are zero, they should return as a miss.
     * For some reason, this isn't properly being reported.
     */

#ifdef KQ_CHEATS
    if (hasCheatEnabled && every_hit_999)
    {
        ta[fighterIndexDefender] = -999;
        return ATTACK_SUCCESS;
    }
#endif

    dmg = mult * base;
    if (dmg == 0)
    {
        dmg = MISS;
        ta[fighterIndexDefender] = dmg;
        return ATTACK_MISS;
    }

    ta[fighterIndexDefender] = 0 - dmg;
    return crit_hit == 1
        ? ATTACK_CRITICAL
        : ATTACK_SUCCESS;
}

void KCombat::battle_render(signed int plyr, size_t hl, int sall)
{
    int a = 0;
    int b = 0;
    int sz = 0;
    int t = 0;
    size_t current_fighter_index = 0;
    size_t fighter_index = 0;

    if (plyr > 0)
    {
        current_fighter_index = plyr - 1;
        curw = fighter[current_fighter_index].fighterImageDatafileWidth;
        fighterImageDatafileX = fighter[current_fighter_index].fighterImageDatafileX;
        fighterImageDatafileY = fighter[current_fighter_index].fighterImageDatafileY;
    }
    else
    {
        fighterImageDatafileX = -1;
        fighterImageDatafileY = -1;
    }

    clear_bitmap(double_buffer);
    blit(backart, double_buffer, 0, 0, 0, 0, KQ_SCREEN_W, KQ_SCREEN_H);

    if ((sall == 0) && (fighterImageDatafileX > -1) && (fighterImageDatafileY > -1))
    {
        draw_sprite(double_buffer, bptr, fighterImageDatafileX + (curw / 2) - 8, fighterImageDatafileY - 8);
        if (current_fighter_index >= MAX_PARTY_SIZE)
        {
            current_fighter_index = plyr - 1;
            t = fighterImageDatafileX + (curw / 2);
            t -= (fighter[current_fighter_index].fighterName.length() * 4);
            int fighterIDy = fighter[current_fighter_index].fighterImageDatafileY;
            int datafileCoords = (fighterIDy < 32
                 ? fighterIDy + fighter[current_fighter_index].fighterImageDatafileHeight
                 : fighterIDy - 32);

            kqDraw.menubox(double_buffer, t - 8, datafileCoords, fighter[current_fighter_index].fighterName.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, t, datafileCoords + 8, fighter[current_fighter_index].fighterName.c_str(), eFontColor::FONTCOLOR_NORMAL);
        }
    }

    auto x_offset = 216;
    for (size_t currentFighterIndex = 0; currentFighterIndex < numchrs; currentFighterIndex++)
    {
        b = currentFighterIndex * x_offset;
        KFighter curFighter = fighter[currentFighterIndex];

        if (curFighter.fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            draw_fighter(currentFighterIndex, (sall == 1));
        }
        else
        {
            curFighter.fighterAttackSpriteFrame = 3;
            draw_fighter(currentFighterIndex, 0);
        }

        kqDraw.menubox(double_buffer, b, 184, 11, 5, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        if (curFighter.fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            sz = bspeed[currentFighterIndex] * 88 / ROUND_MAX;
            if (sz > 88)
            {
                sz = 88;
            }

            a = 116;
            if (curFighter.fighterSpellEffectStats[S_TIME] == 1)
            {
                a = 83;
            }
            else if (curFighter.fighterSpellEffectStats[S_TIME] == 2)
            {
                a = 36;
            }

            a += (sz / 11);
            hline(double_buffer, b + 8, 229, b + sz + 8, a + 1);
            hline(double_buffer, b + 8, 230, b + sz + 8, a);
            hline(double_buffer, b + 8, 231, b + sz + 8, a - 1);
        }

        eFontColor nameTextColor = (hl == currentFighterIndex + 1)
            ? eFontColor::FONTCOLOR_GOLD : eFontColor::FONTCOLOR_NORMAL;
        kqDraw.print_font(double_buffer, b + 8, 192, curFighter.fighterName.c_str(), nameTextColor);

        std::stringstream ss;
        ss << "HP: " << std::setw(3) << curFighter.fighterHealth << "/" << std::setw(3) << curFighter.fighterMaxHealth;
        std::string hpStr = ss.str();
        /*  RB IDEA: If the character has less than 1/5 of his/her max    */
        /*           health points, it shows the amount with red (the     */
        /*           character is in danger). I suggest setting that '5'  */
        /*           as a '#define WARNING_LEVEL 5' or something like     */
        /*           that, so we can change it easily (maybe we can let   */
        /*           the player choose when it should be turned red).     */
        /*  TT TODO: I like this idea; maybe somewhere in the Options     */
        /*           menu?  I find that when the bar flashes red/yellow   */
        /*           to warn the player, it's much more eye-pleasing than */
        /*           just a solid color (and not too hard to implement).  */

        eFontColor healthTextColor = curFighter.isFighterHealthCritical()
            ? eFontColor::FONTCOLOR_RED : eFontColor::FONTCOLOR_NORMAL;
        kqDraw.print_font(double_buffer, b + 8, 208, _(hpStr.c_str()), healthTextColor);

        hline(double_buffer, b + 8, 216, b + 95, 21);
        sz = (curFighter.fighterHealth > 0)
            ? curFighter.fighterHealth * 88 / curFighter.fighterMaxHealth
            : 88;

        hline(double_buffer, b + 8, 216, b + 8 + sz, 12);
        ss.str("");
        ss << "HP: " << std::setw(3) << curFighter.fighterMagic<< "/" << std::setw(3) << curFighter.fighterMaxMagic;
        std::string mpStr = ss.str();

        /*  RB IDEA: Same suggestion as with health, just above. */
        eFontColor magicTextColor = curFighter.isFighterMagicCritical()
            ? eFontColor::FONTCOLOR_RED : eFontColor::FONTCOLOR_NORMAL;
        kqDraw.print_font(double_buffer, b + 8, 218, _(mpStr.c_str()), magicTextColor);
        hline(double_buffer, b + 8, 226, b + 95, 21);
        sz = (curFighter.fighterMagic > 0)
            ? curFighter.fighterMagic * 88 / curFighter.fighterMaxMagic
            : 88;
        hline(double_buffer, b + 8, 226, b + 8 + sz, 12);
        kqDraw.draw_stsicon(double_buffer, 1, currentFighterIndex, 17, b + 8, 200);
    }

    for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + num_enemies; fighter_index++)
    {
        if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            draw_fighter(fighter_index, (sall == 2));
        }
    }

    if (display_attack_string == 1)
    {
        size_t ctext_length = strlen(attack_string) * 4;
        kqDraw.menubox(double_buffer, 152 - ctext_length, 8, strlen(attack_string), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 160 - ctext_length, 16, attack_string, eFontColor::FONTCOLOR_NORMAL);
    }
}

bool KCombat::hasBattleEnded()
{
    // If any players are still alive, enemies have not won.
    bool anyHeroesAlive = false;
    for (size_t fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            anyHeroesAlive = true;
            // No need to continue checking if any are still alive.
            break;
        }
    }

    if (!anyHeroesAlive)
    {
        enemies_win();
        return true;
    }

    // If any enemies are still alive, players have not won.
    bool anyEnemiesAlive = false;
    for (size_t fighter_index = 0; fighter_index < num_enemies; fighter_index++)
    {
        if (fighter[fighter_index + MAX_PARTY_SIZE].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            anyEnemiesAlive = true;
            // No need to continue checking if any are still alive.
            break;
        }
    }

    if (!anyEnemiesAlive)
    {
        heroes_win();
        return true;
    }

    return false;
}

int KCombat::combat(size_t battleIdentifier)
{
    int lc = 0;

#ifdef KQ_CHEATS
    if (hasCheatEnabled && no_monsters)
    {
        return 0;
    }
#endif

    /* PH: some checking! */
    if (battleIdentifier >= NUM_BATTLES)
    {
        //std::string errorStr = "Combat: battle " + std::to_string(battleIdentifier) + " does not exist.";
        // program_death(errorStr.c_str());
        return 1;
    }

    /* TT: no battles during scripted/target movements */
    if (g_ent[0].movemode != MM_STAND)
    {
        return 0;
    }

    int hero_level = party[pidx[0]].lvl;
    size_t encounter = select_encounter(battles[battleIdentifier].etnum, battles[battleIdentifier].eidx);

    /*  RB: check if we had had a random encounter  */
    if (battles[battleIdentifier].enc > 1)
    {
#ifdef KQ_CHEATS
        /* skip battle if no_random_encouters cheat is set */
        if (hasCheatEnabled && no_random_encounters)
        {
            return 0;
        }
#endif

        /* skip battle if haven't moved enough steps since last battle,
         * or if it's just not time for one yet */
        if ((steps < STEPS_NEEDED) || (kqrandom->random_range_exclusive(0, battles[battleIdentifier].enc) > 0))
        {
            return 0;
        }

        /* Likely (not always) skip random battle if repulse is active */
        if (save_spells[P_REPULSE] > 0)
        {
            lc = (hero_level - erows[encounter].lvl) * 20;
            if (lc < 5)
            {
                lc = 5;
            }

            /* Although Repulse is active, there's still a chance of battle */
            if (kqrandom->random_range_exclusive(0, 100) < lc)
            {
                return 0;
            }
        }
    }

    if (hero_level >= erows[encounter].lvl + 5 && battles[battleIdentifier].eidx == 99)
    {
        lc = (hero_level - erows[encounter].lvl) * 5;

        /* TT: This will skip battles based on a random number from hero's
         *     level minus enemy's level.
         */
        if (kqrandom->random_range_exclusive(0, 100) < lc)
        {
            return 0;
        }
    }

    /* Player is about to do battle. */

    steps = 0;
    init_fighters();
    return do_combat(battleIdentifier);
}

void KCombat::do_action(size_t fighter_index)
{
    size_t imb_index;
    uint8_t imbued_item;
    uint8_t spell_type_status;

    for (imb_index = 0; imb_index < 2; imb_index++)
    {
        imbued_item = fighter[fighter_index].imb[imb_index];
        if (imbued_item > 0)
        {
            cast_imbued_spell(fighter_index, imbued_item, 1, TGT_CASTER);
        }
    }

    spell_type_status = fighter[fighter_index].fighterSpellEffectStats[S_MALISON];
    if (spell_type_status > 0)
    {
        if (kqrandom->random_range_exclusive(0, 100) < spell_type_status * 5)
        {
            bIsEtherEffectActive[fighter_index] = false;
        }
    }

    spell_type_status = fighter[fighter_index].fighterSpellEffectStats[S_CHARM];
    if (spell_type_status > 0)
    {
        fighter[fighter_index].fighterSpellEffectStats[S_CHARM]--;
        spell_type_status = fighter[fighter_index].fighterSpellEffectStats[S_CHARM];

        if (fighter_index < MAX_PARTY_SIZE)
        {
            heroc.auto_herochooseact(fighter_index);
        }
        else
        {
            enemy_charmaction(fighter_index);
        }
    }

    if (bIsEtherEffectActive[fighter_index])
    {
        kqDraw.revert_cframes(fighter_index, 0);
        if (fighter_index < MAX_PARTY_SIZE)
        {
            if (spell_type_status == 0)
            {
                heroc.hero_choose_action(fighter_index);
            }
        }
        else
        {
            enemy_chooseaction(fighter_index);
        }
    }

    bIsEtherEffectActive[fighter_index] = false;
    if (hasBattleEnded())
    {
        combatend = EVERYONE_DEFEATED;
    }
}

int KCombat::do_combat(size_t bno)
{
    if (bno < NUM_BATTLES)
    {
        return do_combat(battles[bno].backimg, battles[bno].bmusic, battles[bno].eidx == 99);
    }
    return 0;
}

int KCombat::do_combat(const std::string& backgroundImageName, char* mus, int is_rnd)
{
    in_combat = 1;
    backart = get_cached_image(backgroundImageName);
    if (is_rnd)
    {
        if ((numchrs == 1) && (pidx[0] == AYLA))
        {
            changeYouSurpriseEnemies = kqrandom->random_range_exclusive(1, 101);
            chanceEnemiesSurpriseYou = kqrandom->random_range_exclusive(1, 4);
        }
        else
        {
            if (numchrs > 1 && (Game.in_party(AYLA) < MAXCHRS))
            {
                changeYouSurpriseEnemies = kqrandom->random_range_exclusive(1, 21);
                chanceEnemiesSurpriseYou = kqrandom->random_range_exclusive(1, 6);
            }
            else
            {
                changeYouSurpriseEnemies = kqrandom->random_range_exclusive(1, 11);
                chanceEnemiesSurpriseYou = kqrandom->random_range_exclusive(1, 11);
            }
        }
    }
    else
    {
        changeYouSurpriseEnemies = 10;
        chanceEnemiesSurpriseYou = 10;
    }

    /*  RB: do the zoom at the beginning of the combat.  */
    Music.pause_music();
    Music.set_music_volume((gmvol / 250.0) * 0.75);
    Music.play_music(mus, 0);
    if (stretch_view == 2)
    {
        kqDraw.do_transition(TRANS_FADE_OUT, 2);
        clear_bitmap(double_buffer);
        kqDraw.do_transition(TRANS_FADE_IN, 64);
    }
    else
    {
        static const uint8_t NUM_ZOOM_STEPS = 8;
        /* TT TODO:
         * Change this so when we zoom into the battle, it won't just zoom into the
         * middle of the screen.  Instead, it's going to zoom into the location where the
         * player is, so if he's on the side of the map somewhere...
         */
        std::unique_ptr<Raster> temp(kqDraw.copy_bitmap(nullptr, double_buffer));
        for (auto zoom_step = 0; zoom_step <= NUM_ZOOM_STEPS; zoom_step++)
        {
            Music.poll_music();
            stretch_blit(temp.get(), double_buffer,
                         zoom_step * (KQ_SCREEN_W / 20) + xofs,
                         zoom_step * (KQ_SCREEN_H / 20) + yofs,
                         KQ_SCREEN_W - (zoom_step * (KQ_SCREEN_W / 10)),
                         KQ_SCREEN_H - (zoom_step * (KQ_SCREEN_H / 10)),
                         0, 0,
                         KQ_SCREEN_W, KQ_SCREEN_H);
            kqDraw.blit2screen(xofs, yofs);
        }
    }

    snap_togrid();
    roll_initiative();
    fighterImageDatafileX = 0;
    fighterImageDatafileY = 0;
    vspell = 0;
    combatend = STILL_IN_COMBAT;

    /*  RB: execute combat  */
    do_round();
    Music.set_music_volume(gmvol / 250.0);
    Music.resume_music();
    if (alldead)
    {
        Music.stop_music();
    }

    steps = 0;
    in_combat = 0;
    timer_count = 0;
    return (1);
}

void KCombat::do_round()
{
    size_t fighter_index;

    timer_count = 0;
    while (combatend == STILL_IN_COMBAT)
    {
        if (timer_count >= 10)
        {
            rcount += BATTLE_INC;

            if (rcount >= ROUND_MAX)
            {
                rcount = 0;
            }

            for (fighter_index = 0; fighter_index < MAX_PARTY_SIZE + num_enemies; fighter_index++)
            {
                if (fighter_index >= numchrs && fighter_index < MAX_PARTY_SIZE)
                {
                    bIsEtherEffectActive[fighter_index] = false;
                    continue;
                }

                if (((fighter[fighter_index].fighterSpellEffectStats[S_POISON] - 1) == rcount) && fighter[fighter_index].fighterHealth > 1)
                {
                    int whereAllIsThisUsed = kqrandom->random_range_exclusive(0, fighter[fighter_index].fighterMaxHealth / 20) + 1;

                    if (whereAllIsThisUsed < 2)
                    {
                        whereAllIsThisUsed = 2;
                    }

                    if ((fighter[fighter_index].fighterHealth - whereAllIsThisUsed) < 1)
                    {
                        whereAllIsThisUsed = fighter[fighter_index].fighterHealth - 1;
                    }

                    ta[fighter_index] = whereAllIsThisUsed;
                    display_amount(fighter_index, eFont::FONT_WHITE, 0);
                    fighter[fighter_index].fighterHealth -= whereAllIsThisUsed;
                }

                /*  RB: the character is regenerating? when needed, get a random value (never lower than 5), and increase the character's health by that amount.  */
                if ((fighter[fighter_index].fighterSpellEffectStats[S_REGEN] - 1) == rcount)
                {
                    int whereAllIsThisUsed = kqrandom->random_range_exclusive(0, 5) + (fighter[fighter_index].fighterMaxHealth / 10);

                    if (whereAllIsThisUsed < 5)
                    {
                        whereAllIsThisUsed = 5;
                    }

                    ta[fighter_index] = whereAllIsThisUsed;
                    display_amount(fighter_index, eFont::FONT_YELLOW, 0);
                    adjust_hp(fighter_index, whereAllIsThisUsed);
                }

                /*  RB: the character has ether active?  */
                bIsEtherEffectActive[fighter_index] = true;
                if ((fighter[fighter_index].fighterSpellEffectStats[S_ETHER] > 0) && (rcount == 0))
                {
                    fighter[fighter_index].fighterSpellEffectStats[S_ETHER]--;
                }

                /*  RB: the character is stopped?  */
                if (fighter[fighter_index].fighterSpellEffectStats[S_STOP] > 0)
                {
                    if (pidx[fighter_index] == TEMMIN)
                    {
                        fighter[fighter_index].aux = 0;
                    }

                    if (rcount == 0)
                    {
                        fighter[fighter_index].fighterSpellEffectStats[S_STOP]--;
                    }

                    bIsEtherEffectActive[fighter_index] = false;
                }

                /*  RB: the character is sleeping?  */
                if (fighter[fighter_index].fighterSpellEffectStats[S_SLEEP] > 0)
                {
                    if (pidx[fighter_index] == TEMMIN)
                    {
                        fighter[fighter_index].aux = 0;
                    }

                    if (rcount == 0)
                    {
                        fighter[fighter_index].fighterSpellEffectStats[S_SLEEP]--;
                    }

                    bIsEtherEffectActive[fighter_index] = false;
                }

                /*  RB: the character is petrified?  */
                if (fighter[fighter_index].fighterSpellEffectStats[S_STONE] > 0)
                {
                    if (pidx[fighter_index] == TEMMIN)
                    {
                        fighter[fighter_index].aux = 0;
                    }

                    if (rcount == 0)
                    {
                        fighter[fighter_index].fighterSpellEffectStats[S_STONE]--;
                    }

                    bIsEtherEffectActive[fighter_index] = false;
                }

                if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] = eDeathType::IS_DEAD || fighter[fighter_index].fighterMaxHealth <= 0)
                {
                    if (pidx[fighter_index] == TEMMIN)
                    {
                        fighter[fighter_index].aux = 0;
                    }

                    bspeed[fighter_index] = 0;
                    bIsEtherEffectActive[fighter_index] = false;
                }

                if (bIsEtherEffectActive[fighter_index])
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_TIME] == 0)
                    {
                        bspeed[fighter_index] += nspeed[fighter_index];
                    }
                    else
                    {
                        if (fighter[fighter_index].fighterSpellEffectStats[S_TIME] == 1)
                        {
                            bspeed[fighter_index] += (nspeed[fighter_index] / 2 + 1);
                        }
                        else
                        {
                            bspeed[fighter_index] += (nspeed[fighter_index] * 2);
                        }
                    }
                }
            }

            PlayerInput.readcontrols();
            kqCombat.battle_render(0, 0, 0);
            kqDraw.blit2screen(0, 0);

            for (fighter_index = 0; fighter_index < (MAX_PARTY_SIZE + num_enemies); fighter_index++)
            {
                if ((bspeed[fighter_index] >= ROUND_MAX) && (bIsEtherEffectActive[fighter_index]))
                {
                    do_action(fighter_index);
                    fighter[fighter_index].ctmem = 0;
                    fighter[fighter_index].csmem = 0;
                    bIsEtherEffectActive[fighter_index] = true;
                    bspeed[fighter_index] = 0;
                }

                if (combatend != STILL_IN_COMBAT)
                {
                    return;
                }
            }

            timer_count = 0;
        }

        Game.kq_yield();
    }
}

void KCombat::draw_fighter(size_t fighter_index, size_t dcur)
{
    static const int AUGMENT_STRONGEST = 20;
    static const int AUGMENT_STRONG = 10;
    static const int AUGMENT_NORMAL = 0;

    int xx;
    int yy;
    int ff;
    KFighter* fr = &fighter[fighter_index];

    xx = fr->fighterImageDatafileX;
    yy = fr->fighterImageDatafileY;

    ff = (!fr->fighterAttackSpriteFrame)
        ? fr->fighterSpriteFacing
        : fr->fighterAttackSpriteFrame;

    if (fr->fighterSpellEffectStats[S_STONE] > 0)
    {
        kqDraw.convert_cframes(fighter_index, 2, 12, 0);
    }

    if (fr->fighterSpellEffectStats[S_ETHER] > 0)
    {
        draw_trans_sprite(double_buffer, cframes[fighter_index][ff], xx, yy);
    }
    else
    {
        if (fighter_index < MAX_PARTY_SIZE)
        {
            // Your party
            Raster* shad = new Raster(cframes[fighter_index][ff]->width * 2 / 3, cframes[fighter_index][ff]->height / 4);

            clear_bitmap(shad);
            ellipsefill(shad, shad->width / 2, shad->height / 2, shad->width / 2, shad->height / 2, makecol(128, 128, 128));
            draw_trans_sprite(double_buffer, shad, xx + (shad->width / 3) - 2, yy + cframes[fighter_index][ff]->height - shad->height / 2);
            delete shad;
        }
        else
        {
            // Enemy
            Raster* shad = new Raster(cframes[fighter_index][ff]->width, cframes[fighter_index][ff]->height / 4);

            clear_bitmap(shad);
            ellipsefill(shad, shad->width / 2, shad->height / 2, shad->width / 2, shad->height / 2, makecol(128, 128, 128));
            draw_trans_sprite(double_buffer, shad, xx, yy + cframes[fighter_index][ff]->height - shad->height / 2);
            delete (shad);
        }

        draw_sprite(double_buffer, cframes[fighter_index][ff], xx, yy);
    }

    if (dcur == 1)
    {
        draw_sprite(double_buffer, bptr, xx + (fr->fighterImageDatafileWidth / 2) - 8, yy - 8);
    }

    if ((vspell == 1) && (fighter_index >= MAX_PARTY_SIZE))
    {
        ff = fr->fighterHealth * 30 / fr->fighterMaxHealth;
        if ((fr->fighterHealth > 0) && (ff < 1))
        {
            ff = 1;
        }

        xx += fr->fighterImageDatafileWidth / 2;
        rect(double_buffer, xx - 16, yy + fr->fighterImageDatafileHeight + 2, xx + 15, yy + fr->fighterImageDatafileHeight + 5, 0);
        if (ff > AUGMENT_STRONGEST)
        {
            rectfill(double_buffer, xx - 15, yy + fr->fighterImageDatafileHeight + 3, xx - 15 + ff - 1, yy + fr->fighterImageDatafileHeight + 4, 40);
        }

        else if ((ff <= AUGMENT_STRONGEST) && (ff > AUGMENT_STRONG))
        {
            rectfill(double_buffer, xx - 15, yy + fr->fighterImageDatafileHeight + 3, xx - 15 + ff - 1, yy + fr->fighterImageDatafileHeight + 4, 104);
        }

        else if ((ff <= AUGMENT_STRONG) && (ff > AUGMENT_NORMAL))
        {
            rectfill(double_buffer, xx - 15, yy + fr->fighterImageDatafileHeight + 3, xx - 15 + ff - 1, yy + fr->fighterImageDatafileHeight + 4, 24);
        }
    }
}

void KCombat::enemies_win() const
{
    Music.play_music("rain.s3m", 0);
    kqCombat.battle_render(0, 0, 0);
    /*  RB FIXME: rest()?  */
    kqDraw.blit2screen(0, 0);
    kq_wait(1000);
    std::string defeatStr = party[pidx[0]].playerName + " was defeated!";
    kqDraw.menubox(double_buffer, 152 - (defeatStr.length() * 4), 48, defeatStr.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 160 - (defeatStr.length() * 4), 56, _(defeatStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.blit2screen(0, 0);
    Game.wait_enter();
    kqDraw.do_transition(TRANS_FADE_OUT, 4);
    alldead = 1;
}

int KCombat::fight(size_t attack_fighter_index, size_t defend_fighter_index, int sk)
{
    int a;
    int tx = -1;
    int ty = -1;
    uint32_t f;
    uint32_t ares;
    size_t fighter_index;
    size_t stats_index;

    for (fighter_index = 0; fighter_index < NUM_FIGHTERS; fighter_index++)
    {
        deffect[fighter_index] = 0;
        ta[fighter_index] = 0;
    }

    /*  check the 'sk' variable to see if we are over-riding the  */
    /*  attackers stats with temporary ones... used for skills    */
    /*  and such                                                  */
    if (sk == 0)
    {
        tempa = status_adjust(attack_fighter_index);
    }

    tempd = status_adjust(defend_fighter_index);
    ares = attack_result(attack_fighter_index, defend_fighter_index);
    for (stats_index = 0; stats_index < NUM_SPELL_TYPES; stats_index++)
    {
        fighter[defend_fighter_index].fighterSpellEffectStats[stats_index] = tempd.fighterSpellEffectStats[stats_index];
    }

    /*  RB TODO: rest(20) or vsync() before the blit?  */
    if (ares == 2)
    {
        for (f = 0; f < 3; f++)
        {
            battle_render(defend_fighter_index + 1, 0, 0);
            kqDraw.blit2screen(0, 0);
            kq_wait(20);
            rectfill(double_buffer, 0, 0, KQ_SCREEN_W, KQ_SCREEN_H, 15);
            kqDraw.blit2screen(0, 0);
            kq_wait(20);
        }
    }

    if ((pidx[defend_fighter_index] == TEMMIN) && (fighter[defend_fighter_index].aux == 2))
    {
        fighter[defend_fighter_index].aux = 1;
        a = 1 - defend_fighter_index;
        tx = fighter[defend_fighter_index].fighterImageDatafileX;
        ty = fighter[defend_fighter_index].fighterImageDatafileY;
        fighter[defend_fighter_index].fighterImageDatafileX = fighter[a].fighterImageDatafileX;
        fighter[defend_fighter_index].fighterImageDatafileY = fighter[a].fighterImageDatafileY - 16;
    }

    if (attack_fighter_index < MAX_PARTY_SIZE)
    {
        fighter[attack_fighter_index].fighterAttackSpriteFrame = 7;
    }
    else
    {
        fighter[attack_fighter_index].fighterImageDatafileY += 10;
    }

    fight_animation(defend_fighter_index, attack_fighter_index, 0);
    if (attack_fighter_index < MAX_PARTY_SIZE)
    {
        fighter[attack_fighter_index].fighterAttackSpriteFrame = 0;
    }
    else
    {
        fighter[attack_fighter_index].fighterImageDatafileY -= 10;
    }

    if ((tx != -1) && (ty != -1))
    {
        fighter[defend_fighter_index].fighterImageDatafileX = tx;
        fighter[defend_fighter_index].fighterImageDatafileY = ty;
    }

    if (ta[defend_fighter_index] != MISS)
    {
        ta[defend_fighter_index] = do_shield_check(defend_fighter_index, ta[defend_fighter_index]);
    }

    display_amount(defend_fighter_index, eFont::FONT_DECIDE, 0);
    if (ta[defend_fighter_index] != MISS)
    {
        fighter[defend_fighter_index].fighterHealth += ta[defend_fighter_index];
        if ((fighter[attack_fighter_index].imb_s > 0) && (kqrandom->random_range_exclusive(0, 5) == 0))
        {
            cast_imbued_spell(attack_fighter_index, fighter[attack_fighter_index].imb_s, fighter[attack_fighter_index].imb_a, defend_fighter_index);
        }

        if ((fighter[defend_fighter_index].fighterHealth <= 0) && (fighter[defend_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD))
        {
            fkill(defend_fighter_index);
            death_animation(defend_fighter_index, 0);
        }

        if (fighter[defend_fighter_index].fighterHealth > fighter[defend_fighter_index].fighterMaxHealth)
        {
            fighter[defend_fighter_index].fighterHealth = fighter[defend_fighter_index].fighterMaxHealth;
        }

        if (fighter[defend_fighter_index].fighterSpellEffectStats[S_SLEEP] > 0)
        {
            fighter[defend_fighter_index].fighterSpellEffectStats[S_SLEEP] = 0;
        }

        if ((fighter[defend_fighter_index].fighterSpellEffectStats[S_CHARM] > 0) && (attack_fighter_index == defend_fighter_index))
        {
            fighter[defend_fighter_index].fighterSpellEffectStats[S_CHARM] = 0;
        }

        return 1;
    }

    return 0;
}

void KCombat::fkill(size_t fighter_index)
{
#ifdef KQ_CHEATS
    /* PH Combat cheat: when a hero dies s/he is mysteriously boosted back to full HP. */
    if (hasCheatEnabled && fighter_index < MAX_PARTY_SIZE)
    {
        fighter[fighter_index].fighterHealth = fighter[fighter_index].fighterMaxHealth;
        return;
    }
#endif

    for (size_t spell_index = 0; spell_index < NUM_SPELL_TYPES; spell_index++)
    {
        fighter[fighter_index].fighterSpellEffectStats[spell_index] = 0;
    }

    fighter[fighter_index].fighterSpellEffectStats[S_DEAD] = eDeathType::IS_DEAD;
    fighter[fighter_index].fighterHealth = 0;
    if (fighter_index < MAX_PARTY_SIZE)
    {
        fighter[fighter_index].fighterDefeatItemCommon = 0;
    }

    deffect[fighter_index] = 1;
    bIsEtherEffectActive[fighter_index] = false;
}

uint8_t KCombat::getChanceEnemiesSurpriseYou() const
{
    return chanceEnemiesSurpriseYou;
}

void KCombat::heroes_win()
{
    int tgp = 0;
    size_t fighter_index;
    int b;
    size_t pidx_index;
    int nc = 0;
    int txp = 0;
    int found_item = 0;
    int nr = 0;
    int ent = 0;
    KFighter t1;
    KFighter t2;

    Music.play_music("rend5.s3m", 0);
    kq_wait(500);
    revert_equipstats();
    for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        fighter[fighter_index].fighterAttackSpriteFrame = 4;
    }

    kqCombat.battle_render(0, 0, 0);
    kqDraw.blit2screen(0, 0);
    kq_wait(250);
    for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        if (fighter[fighter_index].fighterSpellEffectStats[S_STONE] == 0 && fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            nc++;
        }

        ta[fighter_index] = 0;
    }

    for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + num_enemies; fighter_index++)
    {
        txp += fighter[fighter_index].fighterExperience;
        tgp += fighter[fighter_index].fighterMoney;
    }

    /*  JB: nc should never be zero if we won, but whatever  */
    if (nc > 0)
    {
        txp /= nc;
    }

    gp += tgp;
    std::string gainedStr;
    if (tgp > 0)
    {
        gainedStr = "Gained " + std::to_string(txp) + " xp and found " + std::to_string(tgp) + " gp.";
    }
    else
    {
        gainedStr = "Gained " + std::to_string(txp) + " xp.";
    }

    kqDraw.menubox(double_buffer, 152 - (gainedStr.length() * 4), 8, gainedStr.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 160 - (gainedStr.length() * 4), 16, _(gainedStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.blit2screen(0, 0);
    fullblit(double_buffer, back);
    for (fighter_index = 0; fighter_index < num_enemies; fighter_index++)
    {
        /* PH bug: (?) should found_item be reset to zero at the start of this loop?
         * If you defeat 2 enemies, you should (possibly) get 2 items, right?
         */
        if (kqrandom->random_range_exclusive(0, 100) < fighter[fighter_index + MAX_PARTY_SIZE].fighterDefeatItemProbability)
        {
            if (fighter[fighter_index + MAX_PARTY_SIZE].fighterDefeatItemCommon > 0)
            {
                found_item = fighter[fighter_index + MAX_PARTY_SIZE].fighterDefeatItemCommon;
            }

            if (fighter[fighter_index + MAX_PARTY_SIZE].fighterDefeatItemRare > 0)
            {
                if (kqrandom->random_range_exclusive(0, 100) < 5)
                {
                    found_item = fighter[fighter_index + MAX_PARTY_SIZE].fighterDefeatItemRare;
                }
            }

            if (found_item > 0)
            {
                if (check_inventory(found_item, 1) != 0)
                {
                    std::string foundStr = std::string(items[found_item].itemName) + " found!";
                    kqDraw.menubox(double_buffer, 148 - (foundStr.length() * 4), nr * 24 + 48, foundStr.length() + 1, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
                    kqDraw.draw_icon(double_buffer, items[found_item].icon, 156 - (foundStr.length() * 4), nr * 24 + 56);
                    kqDraw.print_font(double_buffer, 164 - (foundStr.length() * 4), nr * 24 + 56, _(foundStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
                    nr++;
                }
            }
        }
    }

    if (nr > 0)
    {
        kqDraw.blit2screen(0, 0);
        Game.wait_enter();
        fullblit(back, double_buffer);
    }

    nr = 0;
    for (pidx_index = 0; pidx_index < numchrs; pidx_index++)
    {
        if (party[pidx[pidx_index]].sts[S_STONE] == 0 && party[pidx[pidx_index]].sts[S_DEAD] == eDeathType::NOT_DEAD)
        {
            b = pidx_index * 160;
            player2fighter(pidx[pidx_index], t1);
            if (give_xp(pidx[pidx_index], txp, 0) == 1)
            {
                kqDraw.menubox(double_buffer, b, 40, 18, 9, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
                player2fighter(pidx[pidx_index], t2);
                kqDraw.print_font(double_buffer, b + 8, 48, _("Level up!"), eFontColor::FONTCOLOR_GOLD);
                kqDraw.print_font(double_buffer, b + 8, 56, _("Max HP"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 64, _("Max MP"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 72, _("Strength"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 80, _("Agility"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 88, _("Vitality"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 96, _("Intellect"), eFontColor::FONTCOLOR_NORMAL);
                kqDraw.print_font(double_buffer, b + 8, 104, _("Sagacity"), eFontColor::FONTCOLOR_NORMAL);
                {
                    std::stringstream ss1, ss2;
                    ss1 << std::setw(3) << t1.fighterMaxHealth << ">";
                    kqDraw.print_font(double_buffer, b + 96, 56, ss1.str().c_str(), eFontColor::FONTCOLOR_NORMAL);
                    ss2 << std::setw(3) << t2.fighterMaxHealth;
                    kqDraw.print_font(double_buffer, b + 128, 56, ss2.str().c_str(), eFontColor::FONTCOLOR_GREEN);
                }
                {
                    std::stringstream ss1, ss2;
                    ss1 << std::setw(3) << t1.fighterMaxMagic << ">";
                    kqDraw.print_font(double_buffer, b + 96, 64, ss1.str().c_str(), eFontColor::FONTCOLOR_NORMAL);
                    ss2 << std::setw(3) << t2.fighterMaxMagic;
                    kqDraw.print_font(double_buffer, b + 128, 64, ss2.str().c_str(), eFontColor::FONTCOLOR_GREEN);
                }

                for (int z = 0; z < 5; z++)
                {
                    std::stringstream ss1, ss2;
                    auto fontColor = (t2.fighterStats[z] > t1.fighterStats[z])
                        ? eFontColor::FONTCOLOR_GREEN
                        : eFontColor::FONTCOLOR_NORMAL;
                    ss1 << std::setw(3) << t1.fighterStats[z] << ">";
                    kqDraw.print_font(double_buffer, b + 96, z * 8 + 72, ss1.str().c_str(), eFontColor::FONTCOLOR_NORMAL);
                    ss2 << std::setw(3) << t2.fighterStats[z];
                    kqDraw.print_font(double_buffer, b + 128, z * 8 + 72, ss2.str().c_str(), fontColor);
                }

                nr++;
            }
            else
            {
                kqDraw.menubox(double_buffer, b, 104, 18, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            }

            std::stringstream nextLevelStr;
            nextLevelStr << "Next level " << std::setw(7) << std::right
                << (party[pidx[pidx_index]].next - party[pidx[pidx_index]].xp);
            kqDraw.print_font(double_buffer, b + 8, 112, nextLevelStr.str().c_str(), eFontColor::FONTCOLOR_GOLD);
        }
    }

    kqDraw.blit2screen(0, 0);
    for (pidx_index = 0; pidx_index < numchrs; pidx_index++)
    {
        if (party[pidx[pidx_index]].sts[S_STONE] == 0 && party[pidx[pidx_index]].sts[S_DEAD] == eDeathType::NOT_DEAD)
        {
            ent += learn_new_spells(pidx[pidx_index]);
        }
    }

    if (ent == 0)
    {
        Game.wait_enter();
    }
}

void KCombat::init_fighters()
{
    size_t fighter_index;

    for (fighter_index = 0; fighter_index < NUM_FIGHTERS; fighter_index++)
    {
        deffect[fighter_index] = 0;
        fighter[fighter_index].fighterMaxHealth = 0;
        fighter[fighter_index].aux = 0;
        /* .defend was not initialized; patch supplied by Sam H */
        fighter[fighter_index].fighterWillDefend = 0;
    }

    /* TT: These two are only called once in the game.
     *     Should we move them here?
     */
    heroc.hero_init();
    enemy_init();
    for (fighter_index = 0; fighter_index < (MAX_PARTY_SIZE + num_enemies); fighter_index++)
    {
        nspeed[fighter_index] = (fighter[fighter_index].fighterStats[A_SPD] + 50) / 5;
    }
}

void KCombat::multi_fight(size_t attack_fighter_index)
{
    size_t fighter_index;
    size_t spell_index;
    size_t start_fighter_index;
    size_t end_fighter_index;
    uint32_t deadcount = 0;
    uint32_t killed_warrior[NUM_FIGHTERS];
    // uint32_t ares[NUM_FIGHTERS];

    for (fighter_index = 0; fighter_index < NUM_FIGHTERS; fighter_index++)
    {
        deffect[fighter_index] = 0;
        ta[fighter_index] = 0;
        killed_warrior[fighter_index] = 0;
    }

    if (attack_fighter_index < MAX_PARTY_SIZE)
    {
        // if the attacker is you, target enemies
        start_fighter_index = MAX_PARTY_SIZE;
        end_fighter_index = num_enemies;
    }
    else
    {
        // if the attacker is enemy, target your party
        start_fighter_index = 0;
        end_fighter_index = numchrs;
    }

    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        tempd = status_adjust(fighter_index);
        if ((fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD) && (fighter[fighter_index].fighterMaxHealth > 0))
        {
            // ares[fighter_index] = attack_result(attack_fighter_index, fighter_index);
            for (spell_index = 0; spell_index < NUM_SPELL_TYPES; spell_index++)
            {
                fighter[fighter_index].fighterSpellEffectStats[spell_index] = tempd.fighterSpellEffectStats[spell_index];
            }
        }

        if (ta[fighter_index] != MISS)
        {
            if (ta[fighter_index] != MISS)
            {
                ta[fighter_index] = do_shield_check(fighter_index, ta[fighter_index]);
            }

            fighter[fighter_index].fighterHealth += ta[fighter_index];
            if ((fighter[fighter_index].fighterHealth <= 0) && (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD))
            {
                fighter[fighter_index].fighterHealth = 0;
                killed_warrior[fighter_index] = 1;
            }

            /*  RB: check we always have less health points than the maximum  */
            if (fighter[fighter_index].fighterHealth > fighter[fighter_index].fighterMaxHealth)
            {
                fighter[fighter_index].fighterHealth = fighter[fighter_index].fighterMaxHealth;
            }

            /*  RB: if sleeping, a good hit wakes him/her up  */
            if (fighter[fighter_index].fighterSpellEffectStats[S_SLEEP] > 0)
            {
                fighter[fighter_index].fighterSpellEffectStats[S_SLEEP] = 0;
            }

            /*  RB: if charmed, a good hit wakes him/her up  */
            if (fighter[fighter_index].fighterSpellEffectStats[S_CHARM] > 0 && ta[fighter_index] > 0 && attack_fighter_index == fighter_index)
            {
                fighter[fighter_index].fighterSpellEffectStats[S_CHARM] = 0;
            }
        }
    }

    if (attack_fighter_index < MAX_PARTY_SIZE)
    {
        fighter[attack_fighter_index].fighterAttackSpriteFrame = 7;
    }
    else
    {
        fighter[attack_fighter_index].fighterImageDatafileY += 10;
    }

    fight_animation(start_fighter_index, attack_fighter_index, 1);
    if (attack_fighter_index < MAX_PARTY_SIZE)
    {
        fighter[attack_fighter_index].fighterAttackSpriteFrame = 0;
    }
    else
    {
        fighter[attack_fighter_index].fighterImageDatafileY -= 10;
    }

    display_amount(start_fighter_index, eFont::FONT_DECIDE, 1);
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        if (killed_warrior[fighter_index] != 0)
        {
            fkill(fighter_index);
            deadcount++;
        }
    }

    if (deadcount > 0)
    {
        death_animation(start_fighter_index, 1);
    }
}

void KCombat::roll_initiative()
{
    size_t fighter_index, j;

    if (changeYouSurpriseEnemies == 1 && chanceEnemiesSurpriseYou == 1)
    {
        changeYouSurpriseEnemies = 10;
        chanceEnemiesSurpriseYou = 10;
    }

    for (fighter_index = 0; fighter_index < NUM_FIGHTERS; fighter_index++)
    {
        fighter[fighter_index].csmem = 0;
        fighter[fighter_index].ctmem = 0;
        bIsEtherEffectActive[fighter_index] = true;
        j = ROUND_MAX * 66 / 100;
        if (j < 1)
        {
            j = 1;
        }

        bspeed[fighter_index] = kqrandom->random_range_exclusive(0, j);
    }

    for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        if (chanceEnemiesSurpriseYou == 1)
        {
            bspeed[fighter_index] = ROUND_MAX;
        }
        else if (changeYouSurpriseEnemies == 1)
        {
            bspeed[fighter_index] = 0;
        }
    }

    for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + num_enemies; fighter_index++)
    {
        if (changeYouSurpriseEnemies == 1)
        {
            bspeed[fighter_index] = ROUND_MAX;
        }
        else if (chanceEnemiesSurpriseYou == 1)
        {
            bspeed[fighter_index] = 0;
        }
    }

    rcount = 0;

    /* PH: This should be ok */
    for (fighter_index = 0; fighter_index < NUM_FIGHTERS; fighter_index++)
    {
        if (fighter_index < numchrs || (fighter_index >= MAX_PARTY_SIZE && fighter_index < (MAX_PARTY_SIZE + num_enemies)))
        {
            for (j = 0; j < 2; j++)
            {
                if (fighter[fighter_index].imb[j] > 0)
                {
                    cast_imbued_spell(fighter_index, fighter[fighter_index].imb[j], 1, TGT_CASTER);
                }
            }
        }
    }

    kqCombat.battle_render(-1, -1, 0);
    kqDraw.blit2screen(0, 0);
    if ((changeYouSurpriseEnemies == 1) && (chanceEnemiesSurpriseYou > 1))
    {
        kqDraw.message(_("You have been ambushed!"), 255, 1500, 0, 0);
    }

    if ((changeYouSurpriseEnemies > 1) && (chanceEnemiesSurpriseYou == 1))
    {
        kqDraw.message(_("You've surprised the enemy!"), 255, 1500, 0, 0);
    }
}

void KCombat::snap_togrid()
{
    int hf = 0;
    int mf = 1;
    int a;

    if (changeYouSurpriseEnemies == 1)
    {
        hf = 1;
    }

    if (chanceEnemiesSurpriseYou == 1)
    {
        mf = 0;
    }

    for (size_t fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        fighter[fighter_index].fighterSpriteFacing = hf;
    }

    for (size_t fighter_index = MAX_PARTY_SIZE; fighter_index < (MAX_PARTY_SIZE + num_enemies); fighter_index++)
    {
        fighter[fighter_index].fighterSpriteFacing = mf;
    }

    hf = 170 - (numchrs * 24);
    for (size_t fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        fighter[fighter_index].fighterImageDatafileX = fighter_index * 48 + hf;
        fighter[fighter_index].fighterImageDatafileY = 128;
    }

    a = fighter[MAX_PARTY_SIZE].fighterImageDatafileWidth + 16;
    mf = 170 - (num_enemies * a / 2);
    for (size_t fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + num_enemies; fighter_index++)
    {
        fighter[fighter_index].fighterImageDatafileX = (fighter_index - MAX_PARTY_SIZE) * a + mf;

        if (fighter[fighter_index].fighterImageDatafileHeight < 104)
        {
            fighter[fighter_index].fighterImageDatafileY = 104 - fighter[fighter_index].fighterImageDatafileHeight;
        }
        else
        {
            fighter[fighter_index].fighterImageDatafileY = 8;
        }
    }
}
