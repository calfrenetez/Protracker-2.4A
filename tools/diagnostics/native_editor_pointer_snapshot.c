/* Passive editor-screen observer. No window/input.device/events/MMIO.
 * Copy under LockIBase, then unlock and close before formatting output.
 * Unlike the Workbench V1 probe, a window with no Title is valid data.
 */
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>

struct IntuitionBase *IntuitionBase;
typedef char pointer32[sizeof(void *) == 4 ? 1 : -1];
typedef char word16[sizeof(WORD) == 2 ? 1 : -1];
typedef char long32[sizeof(ULONG) == 4 ? 1 : -1];
typedef char custom_screen_mask[SCREENTYPE == 15 && CUSTOMSCREEN == 15 ? 1 : -1];
typedef char borderless_backdrop_flags[WFLG_BORDERLESS == 0x800 && WFLG_BACKDROP == 0x100 ? 1 : -1];

struct title_copy { unsigned state, length; unsigned char bytes[31]; };

/* 0=absent, 1=empty, 2=complete nonempty text, 3=too long. */
static void copy_title(struct title_copy *out, const UBYTE *title)
{
    unsigned n = 0;
    out->length = 0;
    out->state = 0;
    if (!title) return;
    while (n < 31 && title[n]) { out->bytes[n] = title[n]; ++n; }
    out->length = n;
    out->state = title[n] ? 3 : (n ? 2 : 1);
}

static void hex_title(const struct title_copy *in, char out[63])
{
    static const char digits[] = "0123456789abcdef";
    unsigned n;
    if (!in->length) { out[0] = '-'; out[1] = 0; return; }
    for (n = 0; n < in->length; ++n) {
        out[n * 2] = digits[in->bytes[n] >> 4];
        out[n * 2 + 1] = digits[in->bytes[n] & 15];
    }
    out[in->length * 2] = 0;
}

int main(void)
{
    struct Screen *screen;
    struct Window *window;
    ULONG lock, window_flags = 0;
    WORD width = 0, height = 0, x = 0, y = 0, left = 0, top = 0, dx = 0, dy = 0;
    WORD wl = 0, wt = 0, ww = 0, wh = 0, wx = 0, wy = 0;
    UWORD modes = 0, flags = 0;
    int present = 0, window_present = 0, bl = 0, bt = 0, br = 0, bb = 0;
    struct title_copy screen_title = {0, 0, {0}}, window_title = {0, 0, {0}};
    char screen_hex[63], window_hex[63];
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase) { puts("PT_EDITOR_POINTER_V2 FAIL no_intuition"); return 20; }
    lock = LockIBase(0);
    screen = IntuitionBase->FirstScreen;
    if (screen) {
        present = 1;
        width = screen->Width; height = screen->Height;
        x = screen->MouseX; y = screen->MouseY;
        left = screen->LeftEdge; top = screen->TopEdge;
        dx = screen->ViewPort.DxOffset; dy = screen->ViewPort.DyOffset;
        modes = screen->ViewPort.Modes; flags = screen->Flags;
        /* DefaultTitle is the screen identity supplied by SA_Title. Title can
         * instead reflect the active window's ScreenTitle. */
        copy_title(&screen_title, screen->DefaultTitle);
        window = screen->FirstWindow;
        if (window) {
            window_present = 1;
            wl = window->LeftEdge; wt = window->TopEdge;
            ww = window->Width; wh = window->Height;
            wx = window->MouseX; wy = window->MouseY;
            bl = window->BorderLeft; bt = window->BorderTop;
            br = window->BorderRight; bb = window->BorderBottom;
            window_flags = window->Flags;
            copy_title(&window_title, window->Title);
        }
    }
    UnlockIBase(lock);
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
    if (!present || !window_present || width <= 0 || height <= 0 || ww <= 0 || wh <= 0 ||
        screen_title.state == 3 || window_title.state == 3) {
        puts("PT_EDITOR_POINTER_V2 FAIL invalid_screen_window_or_title"); return 20;
    }
    hex_title(&screen_title, screen_hex); hex_title(&window_title, window_hex);
    printf("PT_EDITOR_SCREEN_V2 screen_width=%d screen_height=%d mouse_x=%d mouse_y=%d "
           "left=%d top=%d viewport_dx=%d viewport_dy=%d viewport_modes=%u screen_flags=%u "
           "default_title_state=%u default_title_hex=%s\n",
           (int)width, (int)height, (int)x, (int)y, (int)left, (int)top, (int)dx, (int)dy,
           (unsigned)modes, (unsigned)flags, screen_title.state, screen_hex);
    printf("PT_EDITOR_WINDOW_V2 left=%d top=%d width=%d height=%d mouse_x=%d mouse_y=%d "
           "border_left=%d border_top=%d border_right=%d border_bottom=%d window_flags=%lu "
           "title_state=%u title_hex=%s\n",
           (int)wl, (int)wt, (int)ww, (int)wh, (int)wx, (int)wy, bl, bt, br, bb,
           (unsigned long)window_flags, window_title.state, window_hex);
    return 0;
}
