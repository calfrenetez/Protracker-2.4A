/* Experimental explicit nix20 startup opt-in; enroll this TU deliberately.
 * Primary basis: AmigaPorts/libnix b3d7d19f147b584b6473185a75b36f9ce127c643,
 * sources/misc/swapstack.c and sources/nix20/steal.me.
 *
 * The typed external anchor requests archive extraction; it never calls the
 * function.  The genuine module registers its own init/exit constructors.
 * A future link must prove anchor/relocation/constructor retention and exclude
 * detach.  Do not replace that check with the presence of __stack alone.
 *
 * 64 KiB is an experimental full-stack request, not a proved aggregate minimum
 * or a shipping choice.  Actual Task startup, restoration, allocation refusal,
 * and application paths remain unqualified until separately exercised.
 */
extern void __stkinit(void);
void (*const pt_editor_stack_init_anchor)(void) = __stkinit;
unsigned long __stack = 65536UL;
