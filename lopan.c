#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <SDL.h>

#include "gfx.h"
#include "font.h"

#define BASEPATH "/switch/SDLLopan/data"

surface tilesgs;

int bgsetnumber;
int tilesetnumber;
char *msg;

char *clickonatile="Click on a tile";
char *clickonanothertile="Click on another tile";

void nomem(int code)
{
	printf("No memory: %d\n",code);
	exit(1);
}
#define TILELOCKED 1
#define TILEGONE 2
#define TILERIGHT1 4
#define TILERIGHT2 8
#define TILETOP 16
#define TILELEFT 32
#define TILEBOTTOM 64
#define TILEBRIGHT 128
#define TILESELECT 256
struct tile
{
	unsigned char x,y;
	int sx,sy;
	unsigned h,t;
	unsigned int flags;
} tiles[144];
int tilecount;

int solution[144];

unsigned char *shadowplane;

void randomize(void)
{
struct timeval tv;
	gettimeofday(&tv,0);
	srand(tv.tv_sec+tv.tv_usec);
}

int removed,backed;

/* depth,x,y */
int layout[]={
1,2,0,
1,4,0,
1,6,0,
1,8,0,
1,10,0,
1,12,0,
1,14,0,
1,16,0,
1,18,0,
1,20,0,
1,22,0,
1,24,0,
1,6,2,
2,8,2,
2,10,2,
2,12,2,
2,14,2,
2,16,2,
2,18,2,
1,20,2,
1,4,4,
1,6,4,
2,8,4,
3,10,4,
3,12,4,
3,14,4,
3,16,4,
2,18,4,
1,20,4,
1,22,4,
1,2,6,
1,4,6,
1,6,6,
2,8,6,
3,10,6,
4,12,6,
4,14,6,
3,16,6,
2,18,6,
1,20,6,
1,22,6,
1,24,6,
1,0,7,
1,26,7,
1,28,7,
-5,13,7,
1,2,8,
1,4,8,
1,6,8,
2,8,8,
3,10,8,
4,12,8,
4,14,8,
3,16,8,
2,18,8,
1,20,8,
1,22,8,
1,24,8,
1,4,10,
1,6,10,
2,8,10,
3,10,10,
3,12,10,
3,14,10,
3,16,10,
2,18,10,
1,20,10,
1,22,10,
1,6,12,
2,8,12,
2,10,12,
2,12,12,
2,14,12,
2,16,12,
2,18,12,
1,20,12,
1,2,14,
1,4,14,
1,6,14,
1,8,14,
1,10,14,
1,12,14,
1,14,14,
1,16,14,
1,18,14,
1,20,14,
1,22,14,
1,24,14,
0,
};

void setfreebits(void)
{
int k,tc,x,y;
unsigned char bitmap[40][40];
struct tile *at;

	memset(bitmap,0,sizeof(bitmap));
	at=tiles;
	tc=0;
	while(tc<144)
	{
		at=tiles+tc++;
		at->flags&=~(TILELOCKED|TILERIGHT1|TILERIGHT2|TILETOP|TILEBOTTOM|TILELEFT);
		if(at->flags&TILEGONE) continue;
		k=1<<at->h;
		x=at->x+2;
		y=at->y+2;
		bitmap[x][y]|=k;
		bitmap[x+1][y]|=k;
		bitmap[x][y+1]|=k;
		bitmap[x+1][y+1]|=k;
	}
	tc=0;
	while(tc<144)
	{
		at=tiles+tc++;
		k=1<<at->h;
		x=at->x+2;
		y=at->y+2;
		if(((bitmap[x-1][y]|bitmap[x-1][y+1]) &
			(bitmap[x+2][y]|bitmap[x+2][y+1]) & k) ||
			((bitmap[x][y]|bitmap[x+1][y]|bitmap[x][y+1]|bitmap[x+1][y+1])
				& (k<<1)))
			at->flags|=TILELOCKED;
		if(!((bitmap[x][y-1]|bitmap[x+1][y-1])&k))
			at->flags|=TILETOP;
		if(!(bitmap[x+2][y]&k))
			at->flags|=TILERIGHT1;
		if(!(bitmap[x+2][y+1]&k))
			at->flags|=TILERIGHT2;
		if(!((bitmap[x-1][y]|bitmap[x-1][y+1])&k))
			at->flags|=TILELEFT;
		if(!((bitmap[x][y+2]|bitmap[x+1][y+2])&k))
			at->flags|=TILEBOTTOM;
	}
}

int intcomp(const void *a,const void *b)
{
	return *(int *)a - *(int *)b;
}
int match(int a,int b)
{
	if(a<34 && b<34)
		return a==b;
	if(a>=36 && a<=39 && b>=36 && b<=39) return 1;
	return a>=40 && a<=43 && b>=40 && b<=43;
}

void scanlayout(int *p)
{
int i,j,k,h,a,b;
int shuffle[144];
int pairs[144];

	for(i=0;i<144;++i)
	{
		if(i<34*4) j=i>>2;
		else j=i-136+36;
		shuffle[i]=j | (rand()&0xffff00);
	}
	qsort(shuffle,144,sizeof(int),intcomp);
	for(i=0;i<144;++i) shuffle[i]&=255;
	k=0;
	for(i=0;i<143;++i)
	{
		a=shuffle[i];
		if(a<0) continue;
		for(j=i+1;j<144;++j)
		{
			b=shuffle[j];
			if(b<0) continue;
			if(match(a,b))
			{
				pairs[k++]=a;
				pairs[k++]=b;
				shuffle[i]=-1;
				shuffle[j]=-1;
				break;
			}
		}
	}

	p=layout;
	tilecount=0;
	while((h=*p++))
	{
		i=*p++;
		j=*p++;
		if(h<0)
		{
			tiles[tilecount].x=i;
			tiles[tilecount].y=j;
			tiles[tilecount].h=-1-h;
			tiles[tilecount].t=pairs[tilecount];
			tiles[tilecount].flags=0;
			++tilecount;
		} else
			for(k=0;k<h;++k)
			{
				tiles[tilecount].x=i;
				tiles[tilecount].y=j;
				tiles[tilecount].h=k;
				tiles[tilecount].t=pairs[tilecount];
				tiles[tilecount].flags=0;
				++tilecount;
			}
	}
retry:
	for(i=0;i<144;++i)
		tiles[i].flags=0;
	k=0;
	while(k<144)
	{
		setfreebits();
		j=0;
		for(i=0;i<144;++i)
		{
			if(tiles[i].flags&(TILEGONE|TILELOCKED)) continue;
//printf("%d:(%d,%d,%d)\n",j,tiles[i].x,tiles[i].y,tiles[i].h);
			shuffle[j++]=i | (rand()&0xffff00);
		}
		if(j<2) goto retry;
		qsort(shuffle,j,sizeof(int),intcomp);
//printf("%d:%d free:taking %d and %d\n",k,j,shuffle[0]&255,shuffle[1]&255);
		i=shuffle[0]&255;
		solution[k]=i;
		tiles[i].flags|=TILEGONE;
		tiles[i].t=pairs[k++];
		i=shuffle[1]&255;
		solution[k]=i;
		tiles[i].flags|=TILEGONE;
		tiles[i].t=pairs[k++];
	}
	for(i=0;i<144;++i)
		tiles[i].flags&=~TILEGONE;
	removed=0;
	backed=0; // change this to 72 to have solution visible on startup...
}

void clearshadowplane(void)
{
	memset(shadowplane,0,(vxsize+7)*vysize>>3);
}

void shadowdot(unsigned int x,unsigned int y,int onoff)
{
unsigned char *p,bit;
	if(x>=vxsize || y>=vysize) return;
	p=shadowplane+(x>>3)+y*((vxsize+7)>>3);
	bit=1<<(x&7);
	if(onoff)
		*p|=bit;
	else
		*p&=~bit;
}

void shadowrect(int x,int y,int sizex,int sizey,int onoff)
{
int i;
	if(y<0)
	{
		sizey+=y;
		y=0;
	}
	if(x<0)
	{
		sizex+=x;
		x=0;
	}
	if(x+sizex>vxsize)
		sizex=vxsize-x;
	if(y+sizey>vysize)
		sizey=vysize-y;
	while(sizey-->0)
	{
		i=sizex;
		while(i-->0)
			shadowdot(x+i,y,onoff);
		++y;
	}
}

void shadowsolidrect(int x,int y,int xsize,int ysize,int rgb)
{
	solidrect(x,y,xsize,ysize,rgb>>16,rgb>>8,rgb);
	shadowrect(x,y,xsize,ysize,0);
}
#define BORDER 0x303030

void doit(void)
{
int i,j,k,h,h2;
int x,y,flags;

	copyfromback(0);
	drawprintfxy(vxsize-140,vysize-80,msg);

	setfreebits();
	clearshadowplane();
	for(i=0;i<144;++i)
	{
		j=tiles[i].h<<2;
		tiles[i].sx=tiles[i].x*21+2+j ;
		tiles[i].sy=tiles[i].y*24+10-j;// + 400;
	}
	for(h=0;h<7;++h)
	{
		for(i=0;i<144;++i)
		{
			h2=tiles[i].h;
			if(h2<h) continue;
			if(tiles[i].flags&TILEGONE) continue;
			h2=(h2-h)<<3;
			x=tiles[i].sx+h2;
			y=tiles[i].sy-h2;
			flags=tiles[i].flags;
			if(flags&TILETOP)
			{
				shadowrect(x+6,y-8,40,1,1);
				shadowrect(x+4,y-7,43,1,1);
				shadowrect(x+2,y-6,43,1,1);
				shadowrect(x+1,y-5,43,1,1);
				shadowrect(x,y-4,43,1,1);
				shadowrect(x-1,y-3,43,1,1);
				shadowrect(x-2,y-2,43,1,1);
				shadowrect(x-3,y-1,43,1,1);
			}
			if(flags&TILERIGHT1)
			{
				shadowrect(x+46,y-6,2,23,1);
				shadowrect(x+45,y-6,1,24,1);
				shadowrect(x+44,y-5,1,24,1);
				shadowrect(x+43,y-4,1,24,1);
				shadowrect(x+42,y-3,1,24,1);
				shadowrect(x+41,y-2,1,24,1);
				shadowrect(x+40,y-1,1,24,1);
			}
			if(flags&TILERIGHT2)
			{
				shadowrect(x+47,y+24-8,1,24,1);
				shadowrect(x+46,y+24-7,1,24,1);
				shadowrect(x+45,y+24-6,1,24,1);
				shadowrect(x+44,y+24-5,1,24,1);
				shadowrect(x+43,y+24-4,1,24,1);
				shadowrect(x+42,y+24-3,1,24,1);
				shadowrect(x+41,y+24-2,1,24,1);
				shadowrect(x+40,y+24-1,1,24,1);
			}
			shadowrect(x+38,y,2,2,1);
		}

		for(i=0;i<tilecount;++i)
		{
			if(tiles[i].h!=h) continue;
			flags=tiles[i].flags;
			if(flags&TILEGONE) continue;
			x=tiles[i].sx;
			y=tiles[i].sy;
			k=tiles[i].t;
			if(flags&TILELEFT)
			{
				shadowsolidrect(x-4,y+2,2,48,0xe7e3c7);
				shadowsolidrect(x-6,y+4,2,48,0xf7f3f7);
			}
			shadowsolidrect(x+38,y+2,2,48,BORDER);
			shadowsolidrect(x-2,y,42,2,BORDER);
			if(flags&TILEBOTTOM)
			{
				shadowsolidrect(x-2,y+48,40,2,0xe7e3c7);
				shadowsolidrect(x-4,y+50,40,2,0xf7f3f7);
			} else
			{
				shadowsolidrect(x-2,y+48,42,2,BORDER);
			}
			gstoback(x-2,y+2,&tilesgs,42*(k%9),48*(k/9),40,46);
			if(flags&(TILEBRIGHT|TILESELECT))
				if(!(flags&TILEBRIGHT) || !(flags&TILESELECT))
					lightenrect(x-2,y+2,40,46);
			shadowrect(x-2,y+2,40,46,0);
		}
	}
	applyshadowplane(shadowplane);
}

void newbgset(void)
{
int i,j,sx,sy;
char temp[64];
surface bgset;

	for(;;)
	{
		++bgsetnumber;
		sprintf(temp,"%s/bg%d.pcx",BASEPATH,bgsetnumber);
		i=readpcx(temp,&bgset);
		if(i)
		{
			if(!bgsetnumber) return;
			bgsetnumber=-1;
		} else
		{
			j=0;
			sy=bgset.ysize;
			while(j<vysize)
			{
				i=0;
				if(j+sy>vysize) sy=vysize-j;
				sx=bgset.xsize;
				while(i<vxsize)
				{
					if(i+sx>vxsize) sx=vxsize-i;
					gstoback(i,j,&bgset,0,0,sx,sy);
					i+=sx;
				}
				j+=sy;
			}
			copytoback(0);
			freegs(&bgset);
			break;
		}
	}
}
int newtileset(void)
{
char temp[64];
int err;

	freegs(&tilesgs);
	for(;;)
	{
		++tilesetnumber;
		sprintf(temp,"%s/tiles%d.pcx",BASEPATH,tilesetnumber);
		err=readpcx(temp,&tilesgs);
		if(!err) return 0;
		else if(!tilesetnumber) return err;
		else tilesetnumber=-1;
	}
}

int processmouse(int code,int mx,int my)
{
int i,j,x,y;

//	mx = mx/1.2 - 80;
	
	if(code!=MYMOUSE1DOWN) return 0;
	j=-1;
	for(i=0;i<144;++i)
	{
		x=tiles[i].sx;
		y=tiles[i].sy;
		if(mx>=x && mx<x+40 && my>=y && my<y+46)
		{
			if(tiles[i].flags&(TILELOCKED|TILEGONE)) continue;
			if(j<0 || tiles[i].h>tiles[j].h)
				j=i;
		}
	}
	if(j<0 || (tiles[j].flags&TILELOCKED)) return 0;
	tiles[j].flags^=TILESELECT;
	if(tiles[j].flags&TILESELECT)
	{
		for(i=0;i<144;++i)
			if(i!=j && (tiles[i].flags&TILESELECT))
				break;
		if(i<144)
		{
			if(match(tiles[i].t,tiles[j].t))
			{
				backed=0;
				solution[removed++]=i;
				solution[removed++]=j;
				tiles[i].flags|=TILEGONE;
				tiles[j].flags|=TILEGONE;
				tiles[i].flags&=~TILESELECT;
				msg=clickonatile;
			}
			tiles[j].flags&=~TILESELECT;
		} else msg=clickonanothertile;
	} else
		msg=clickonatile;
	return 1;
}

int main(int argc, char **argv)
{
int i;
int code;
unsigned char redraw;
int backup;

	bgsetnumber=tilesetnumber=-1;
	randomize();
	opendisplay(640,400);
	atexit(closedisplay);
	initfont();
	tilesgs.pic=0;
	if((i=newtileset()))
	{
		printf("Failed to load a tile set, code %d\n",i);
		exit(2);
	}
	shadowplane=malloc((vxsize+7)*vysize>>3);
	if(!shadowplane) nomem(20);

/*
	for(j=0;j<vysize;++j)
		for(i=0;i<vxsize;++i)
			if((rand()&255)<64)
				rgbdot(i,j,0x51,0x71,0x10);
			else
				rgbdot(i,j,0x00,0x71,0x00);
*/
	solidrect(0,0,vxsize,vysize,0x3c,0x54,0x0c);
	msg=clickonatile;

	copytoback(0);
//	newbgset();

	scanlayout(layout);
	redraw=1;
	backup=0;
	while(!exitflag)
	{
		delay(1);

		while(scaninput(),(code=nextcode())>=0)
		{
			if(code&MYMOUSE)
			{
				int x,y;
				x=nextcode();
				y=nextcode();
				redraw|=processmouse(code,x,y);
			} else if(code==13)
			{
				scanlayout(layout);
				i=0;
				redraw=1;
			} else if(code==MYLEFT) backup=1;
			else if(code==MYRIGHT) backup=-1;
			else if(code==MYF1)
			{
				newtileset();
				redraw=1;
			}
			else if(code==MYF2)
			{
				newbgset();
				redraw=1;
			} else if(code==0x1b) exitflag=1;
		}
//		if(redraw) {doit();copyup();redraw=0;}
		if(redraw) {doit();copyup();redraw=1;}
		if(backup)
		{
			if(backup>0)
			{
				if(removed)
				{
					tiles[solution[--removed]].flags&=~TILEGONE;
					tiles[solution[--removed]].flags&=~TILEGONE;
					++backed;
					redraw=1;
				}
			} else
			{
				if(backed)
				{
					--backed;
					tiles[solution[removed++]].flags|=TILEGONE;
					tiles[solution[removed++]].flags|=TILEGONE;
					redraw=1;
				}
			}
			backup=0;
		}
	}

	return 0;

}
