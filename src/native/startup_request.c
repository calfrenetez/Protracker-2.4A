/* Explicit app-owned libnix nix20 request provider, not a general GUI hook.
 * Ordinary C ABI matches pinned void __request(const char *). May run before
 * main on its original stack. Never use GUI, stdio, heap, callbacks or exit:
 * the genuine caller owns its subsequent return/exit policy. Message, if not
 * NULL, follows original libnix contract: a live readable C string. */
#include <proto/dos.h>
void __request(const char *message)
{
    static const char prefix[]="PT24GEdit: ";
    static const char newline[]="\n";
    LONG length=0;
    BPTR output;
    /* Read only the CRT-owned slot. It may still be the original -1 sentinel
     * before init; never open, replace, clear or close any library here. */
    if(!DOSBase || DOSBase==(struct DosLibrary *)-1L)return;
    output=Output();
    if(!output)return;
    if(message)while(length<160 && message[length])++length;
    (void)Write(output,prefix,(LONG)(sizeof(prefix)-1));
    if(length)(void)Write(output,message,length);
    (void)Write(output,newline,1);
}
