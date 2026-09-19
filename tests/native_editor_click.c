/* Enhanced editor disposable-emulator probe: genuine input.device events, never CIA writes.
 * Exit success proves delivery and released physical buttons; the separate
 * screenshot/log must show the requested control to prove consumption.
 */
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct IntuitionBase *IntuitionBase;

static int send_event(struct IOStdReq *io, struct InputEvent *event)
{
    io->io_Command = IND_WRITEEVENT;
    io->io_Data = event;
    io->io_Length = sizeof(*event);
    return DoIO((struct IORequest *)io) == 0;
}

int main(int argc,char **argv)
{
    struct MsgPort *port = NULL;
    struct IOStdReq *io = NULL;
    struct Screen *screen = NULL;
    struct InputEvent event;
    struct IEPointerPixel pixel;
    unsigned before = 0, down = 0, after = 0;
    int opened = 0, rc = 20, attempt,x,y;
    char *end;long value;
    if(argc!=3)return 20;
    value=strtol(argv[1],&end,10);if(*end || value<0 || value>=640)return 20;x=(int)value;
    value=strtol(argv[2],&end,10);if(*end || value<0 || value>=512)return 20;y=(int)value;
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase) goto done;
    for (attempt = 0; attempt < 40; ++attempt) {
        ULONG lock = LockIBase(0);
        screen = IntuitionBase->FirstScreen;
        if (!screen || screen->Width != 640 || screen->Height != 512 ||
            !screen->Title || strcmp((char *)screen->Title, "ProTracker 2.4G Enhanced Editor"))
            screen = NULL;
        UnlockIBase(lock);
        if (screen) break;
        Delay(25);
    }
    if (!screen) goto done;
    port = CreateMsgPort();
    if (!port) goto done;
    io = (struct IOStdReq *)CreateIORequest(port, sizeof(*io));
    if (!io || OpenDevice("input.device", 0, (struct IORequest *)io, 0)) goto done;
    opened = 1;
    memset(&event, 0, sizeof(event));
    memset(&pixel, 0, sizeof(pixel));
    pixel.iepp_Screen = screen;
    pixel.iepp_Position.X = x;
    pixel.iepp_Position.Y = y;
    event.ie_Class = IECLASS_NEWPOINTERPOS;
    event.ie_SubClass = IESUBCLASS_PIXEL;
    event.ie_EventAddress = &pixel;
    if (!send_event(io, &event)) goto done;
    Delay(10);
    before = *(volatile unsigned char *)0xbfe001 & 0x40;
    memset(&event, 0, sizeof(event));
    event.ie_Class = IECLASS_RAWMOUSE;
    event.ie_Code = IECODE_LBUTTON;
    event.ie_Qualifier = IEQUALIFIER_LEFTBUTTON;
    if (!send_event(io, &event)) goto done;
    Delay(10);
    down = *(volatile unsigned char *)0xbfe001 & 0x40;
    event.ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
    event.ie_Qualifier = 0;
    if (!send_event(io, &event)) goto done;
    Delay(10);
    after = *(volatile unsigned char *)0xbfe001 & 0x40;
    if (before == 0x40 && down == 0x40 && after == 0x40) rc = 0;
done:
    /* Always release our synthetic press on any partial failure. */
    if (opened) {
        memset(&event, 0, sizeof(event));
        event.ie_Class = IECLASS_RAWMOUSE;
        event.ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
        send_event(io, &event);
        CloseDevice((struct IORequest *)io);
    }
    if (io) DeleteIORequest((struct IORequest *)io);
    if (port) DeleteMsgPort(port);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    printf("EDITOR MOUSE PROBE %s CIA before=%u down=%u after=%u; inspect editor control acknowledgement\n",
           rc ? "FAIL" : "PASS", before, down, after);
    return rc;
}
