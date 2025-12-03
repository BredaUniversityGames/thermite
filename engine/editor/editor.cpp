/* this file asserts if its included when the game is built */
#ifdef THERMITE_EDITOR
static_assert(true, "We are running editor");
#else
static_assert(false, "We are running game");
#endif