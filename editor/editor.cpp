#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
/* Shouldn't happen */
static_assert(false, "THERMITE_EDITOR must be defined to 1 in editor builds");
#endif
