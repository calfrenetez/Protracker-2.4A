#include <pt_host_intuition.h>
struct Window *OpenWindowTags(void *,...);
void CloseWindow(struct Window *);
void BeginRefresh(struct Window *);
void EndRefresh(struct Window *,int);
