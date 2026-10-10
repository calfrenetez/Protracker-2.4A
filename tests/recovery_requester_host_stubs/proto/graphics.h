#include <pt_host_intuition.h>
struct TextFont *OpenFont(struct TextAttr *);
void CloseFont(struct TextFont *);
void SetFont(struct RastPort *,struct TextFont *);
void Move(struct RastPort *,int,int);
void Draw(struct RastPort *,int,int);
void Text(struct RastPort *,STRPTR,ULONG);
void SetAPen(struct RastPort *,unsigned);
void SetBPen(struct RastPort *,unsigned);
void SetDrMd(struct RastPort *,unsigned);
void RectFill(struct RastPort *,int,int,int,int);
void WaitBlit(void);
