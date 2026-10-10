#ifndef PT_HOST_INTUITION_STUB_H
#define PT_HOST_INTUITION_STUB_H
#include <stdint.h>
#include <stddef.h>
/* HOST shapes only: pointer-sized ULONG avoids truncating the native source's
 * tag pointer casts on a 64-bit HOST. This does not qualify Amiga ABI widths. */
typedef int32_t LONG;
typedef uintptr_t ULONG;
typedef uint16_t UWORD;
typedef intptr_t BPTR;
typedef char *STRPTR;
struct Screen {unsigned identity;};
struct RastPort {unsigned identity;};
struct MsgPort {unsigned mp_SigBit;};
struct Message {unsigned identity;};
struct Window {struct Screen *WScreen;struct RastPort *RPort;struct MsgPort *UserPort;
    int BorderLeft,BorderTop;ULONG IDCMPFlags;};
struct IntuiMessage {struct Message ExecMessage;ULONG Class;UWORD Code,Qualifier;
    int MouseX,MouseY;};
struct TextFont {unsigned tf_XSize,tf_YSize;};
struct TextAttr {STRPTR ta_Name;UWORD ta_YSize;unsigned char ta_Style,ta_Flags;};
#define TRUE 1
#define FALSE 0
#define JAM2 2
#define IDCMP_VANILLAKEY ((ULONG)1)
#define IDCMP_RAWKEY ((ULONG)2)
#define IDCMP_MOUSEBUTTONS ((ULONG)4)
#define IDCMP_CLOSEWINDOW ((ULONG)8)
#define IDCMP_REFRESHWINDOW ((ULONG)16)
#define SELECTDOWN 0x68
#define SELECTUP 0xe8
#define MENUDOWN 0x69
#define TAG_DONE 0
#define WA_CustomScreen 101
#define WA_Left 102
#define WA_Top 103
#define WA_InnerWidth 104
#define WA_InnerHeight 105
#define WA_Title 106
#define WA_DragBar 107
#define WA_CloseGadget 108
#define WA_Activate 109
#define WA_RMBTrap 110
#define WA_SmartRefresh 111
#define WA_IDCMP 112
#endif
