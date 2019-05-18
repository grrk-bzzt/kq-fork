#pragma once

/*! \file
 * \brief Hero combat header file
 */

 /*! \name Hero identifiers */
enum ePIDX
{
    PIDX_UNDEFINED = -1,
    SENSAR = 0,
    SARINA = 1,
    CORIN = 2,
    AJATHAR = 3,
    CASANDRA = 4,
    TEMMIN = 5,
    AYLA = 6,
    NOSLOM = 7,

    MAXCHRS,

    SEL_ALL_ALLIES,
    SEL_ALL_ENEMIES
};

class KHero
{
public:
    KHero();
    ~KHero();

    /*! \brief Set up heroes for combat
     *
     * This sets up the heroes' fighter vars and frames.
     * The frames are:
     * - Facing away
     * - Facing towards
     * - Arms out
     * - Dead
     * - Victory
     * - Arms forward
     *
     * Then an array to the right where each character is wielding some different luminous green weapons.
     * These colors are replaced by the 'true' weapon colous as determined by s_item::kol.
     * The shape is chosen by KFighter::current_weapon_type
     */
    void hero_init();

    /*! \brief Show menu for action selection
     *
     * Give the player a menu for a specific character and
     * allow him/her to choose an action.
     *
     * \param   fighter_index Index of player (see constants in progress.h)
     */
    void hero_choose_action(size_t fighter_index);

    /*! \brief Auto-choose options for confused player
     *
     * Chooses actions for the character when s/he is charmed/confused.  This is
     * pretty much the same as the enemy_charmaction function and I really should
     * incorporate them into one.
     *
     * \todo    Incorporate enemy_charmaction
     *
     * \param   who Index of player (see constants in progress.h)
     */
    void auto_herochooseact(int who);

    /*! \brief Count available spells
     *
     * This checks a fighter's list of spells to see if there
     * are any for her/him to cast.
     *
     * \param   who Index of Hero in your party
     * \returns the number of available spells
     */
    int available_spells(int who);

    /*! \brief Choose spell
     *
     * Draw the character's spell list and then choose a spell.
     *
     * \param   c Character id
     * \returns 0 if canceled or 1 if something happened
     */
    int combat_spell_menu(int c);

    /**
     * Enable or disable the use of the Item option during battles.
     *
     * Some battles prohibit the use of the Item option during battles (such as the
     * coliseum).
     *
     * @param canUseItem True to allow items during battle, false to disallow it.
     */
    void allowItemUseDuringBattle(bool canUseItem);

    /**
     * Get whether the use of the Item option during battle is allowed.
     *
     * @return True if Item option is given to player, false if not.
     */
    bool canItemBeUsedDuringBattle() const;

private:
    /*! \brief Select target for hero
     *
     * Select a target for the hero to attack.
     *
     * \param   whom Index of player (see constants in progress.h)
     * \returns index of target
     */
    int hero_attack(int whom);

    /*! \brief Display item list
     *
     * This displays a list of the heroes items for use in combat.
     *
     * \param   pg The item list's current page
     */
    void combat_draw_items(int pg);

    /*! \brief Choose combat item
     *
     * This is the menu used to display the hero's items in combat and to allow
     * him/her to select one.  The player then selects the target and the action
     * is performed.
     *
     * \param   whom Index of character who is doing the choosing
     * \returns 0 if cancelled, 1 if item was chosen
     */
    int combat_item_menu(int whom);

    /*! \brief Can item be used in combat
     *
     * This tells us whether or not the specified item is usable in combat.
     *
     * \param   itno Index of item
     * \returns 1 item can be used, otherwise 0
     */
    int combat_item_usable(int itno);

    /*! \brief Use item
     *
     * Use the selected item and show the effects.
     *
     * \param   ss Index of character attacking or PSIZE if an enemy is attacking
     * \param   t1 Item to use
     * \param   tg Index of target
     * \returns 1 if anything happened, 0 otherwise
     */
    int combat_item(int ss, int t1, int tg);

    /*! \brief Draw equipment list
     *
     * Draw the character's list of equipment.
     *
     * \param   dud Index of party member to draw
     */
    void draw_invokable(int dud);

    /*! \brief Is item invokable
     *
     * Tells us whether or not a specified item is invokable.
     *
     * \param   t1 Index of item
     * \returns 1 if item can be invoked, 0 otherwise
     */
    int can_invoke_item(int t1);

    /*! \brief Display and choose item
     *
     * Displays the characters list of equipment and which ones are invokable.
     * The player may then choose one (if any) to invoke.
     *
     * \param   whom Index of character
     * \returns 1 if item was selected, 0 if cancelled
     */
    int hero_invoke(int whom);

    /*! \brief Invoke hero item
     *
     * Invoke the specified item according to target.
     * Calls select_hero or select_enemy as required.
     * \note Includes fix for bug (SF.net) "#858657 Iron Rod Multiple Target Fizzle"
     *       aka (Debian) "#224521 Multitargeting with iron rod crashes"
     *       submitted by Sam Hocevar
     *
     * \param   attacker_fighter_index Index of target in Hero's party
     * \param   item_index Item that is being invoked
     * \returns 1 if item was successfully used, 0 otherwise
     */
    int hero_invokeitem(size_t attacker_fighter_index, size_t item_index);

    /*! \brief Can heroes run?
     *
     * Check whether or not the heroes can run, and then display
     * the little running-away sequence.
     */
    void hero_run();

    /*! \brief Draw spell list
     *
     * Draw the list of spells that the character can use in combat.
     *
     * \param   c Character id
     * \param   ptr The current line of the menu pointer
     * \param   pg The current page in the spell list
     */
    void combat_draw_spell_menu(int c, int ptr, int pg);

    /*! \brief Check spell targeting
     *
     * Perform the necessary checking to determine target selection for the
     * particular character's spell.
     *
     * \param   whom Character id
     * \returns -1 if the spell has no targeting,
     *          0 if cancelled
     *          1 if target selected
     */
    int combat_spell_targeting(int whom);

    /*! \brief Check spell is castable
     *
     * Perform the necessary checking to determine if a spell can be
     * cast in combat and if the mp exists to do so.
     *
     * \param   spell_caster Character id
     * \param   spell_number Spell id
     * \returns 1 if spell can be cast, 0 if not
     */
    int combat_castable(int spell_caster, int spell_number);

private:
    /* Text names of hero skills */
    char sk_names[ePIDX::MAXCHRS][7];

    bool m_CanUseItem;
};

// Global hero class
extern KHero heroc;
