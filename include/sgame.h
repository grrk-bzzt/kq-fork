#pragma once

#include <vector>

#include "kq.h"

class KSaveGame
{
public:
    KSaveGame();

    ~KSaveGame();

    /*! \brief Load mini stats
     *
     * This loads the mini stats for each saved game.
     * These mini stats are just for displaying info about the save game on the
     * save/load game screen.
     */
    void load_sgstats();

    /*! \brief Main menu screen
     *
     * This is the main menu... just display the opening and then the menu and
     * then wait for input.  Also handles loading a saved game, and the config menu.
     *
     * \param   c zero if the splash (the bit with the staff and the eight heroes)
     *            should be displayed.
     * \returns 1 if new game, 0 if continuing, 2 if exit
     */
    int start_menu(int skip_splash);

    /*! \brief Display system menu
     *
     * This is the system menu that is invoked from within the game.
     * From here you can save, load, configure a couple of options or
     * exit the game altogether.
     * \date 20040229 PH added 'Save anytime' facility when cheat mode is ON
     *
     * \returns 0 if cancelled or nothing happened, 1 otherwise
     */
    int system_menu();

protected:
    /*! \brief Display saved game statistics
     *
     * This is the routine that displays the information about
     * each saved game for the save/load screen.
     *
     * \param   saving 0 if loading, 1 if saving.
     */
    void show_sgstats(int saving);

    /*! \brief Save game
     *
     * You guessed it... save the game.
     *
     * \returns 0 if save failed, 1 if success
     */
    int save_game();

    /*! \brief Load game
     *
     * Uh-huh.
     * PH 20030805 Made endian-safe
     * PH 20030914 Now ignores keyboard settings etc in the save file
     * \returns 1 if load succeeded, 0 otherwise
     */
    int load_game();

    /*! \brief Delete game
     *
     * You guessed it... delete the game.
     */
    void delete_game();

    /*! \brief Save/Load menu
     *
     * This is the actual save/load menu.  The only parameter to
     * the function indicates whether we are saving or loading.
     *
     * \param   am_saving 0 if loading, 1 if saving
     * \returns 0 if an error occurred or save/load cancelled
     */
    int saveload(int am_saving);

    /*! \brief Confirm save
     *
     * If the save slot selected already has a saved game in it, confirm that we
     * want to overwrite it.
     *
     * \returns 0 if cancelled, 1 if confirmed
     */
    int confirm_action();

protected:
    /* Number of save game slots */
    static const size_t NUMSG;

    /* These describe the save slots. Number of characters, gp, etc */
    /* They are used to make the save menu prettier. */
    std::vector<s_sgstats> savegame;

    /* Which save_slot the player is pointing to */
    size_t save_ptr = 0;

    /* Which save_slot is shown at the top of the screen (for scrolling) */
    size_t top_pointer = 0;

    /* Maximum number of slots to show on screen. */
    int max_onscreen = 5;
};

extern KSaveGame SaveGame;
