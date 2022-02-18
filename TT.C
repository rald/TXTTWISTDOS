#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <conio.h>
#include <dos.h>
#include <sys/nearptr.h>



#include "font.h"
#include "bicycle.h"



#define VIDEO_INT           0x10  /* the BIOS video interrupt. */
#define WRITE_DOT           0x0C  /* BIOS func to plot a pixel. */
#define SET_MODE            0x00  /* BIOS func to set the video mode. */
#define VGA_256_COLOR_MODE  0x13  /* use to set 256-color mode. */
#define TEXT_MODE           0x03  /* use to set 80x25 text mode. */

#define PALETTE_INDEX       0x03C8
#define PALETTE_DATA        0x03C9
#define INPUT_STATUS        0x03DA
#define VRETRACE            0x08

#define SCREEN_WIDTH        320   /* width in pixels of mode 0x13 */
#define SCREEN_HEIGHT       200   /* height in pixels of mode 0x13 */
#define NUM_COLORS          256   /* number of colors in mode 0x13 */


#define DELAY_MAX 10
#define WORDLIST_FILE "wordlist.txt"
#define LINE_MAX 2048

#define VKEY_BSP  8
#define VKEY_ENT 13
#define VKEY_ESC 27
#define VKEY_SPC 32



typedef unsigned char  byte;           
typedef unsigned short word;

byte *VGA = (byte *)0xA0000;      /* this points to video memory. */
word *my_clock = (word *)0x046C;  /* this points to the 18.2hz system clock. */

byte dbuf[SCREEN_WIDTH*SCREEN_HEIGHT];



typedef struct Ball Ball;

struct Ball {
  char letter;
  double x,y,r;
  double dx,dy;
  bool isMoving;
  bool isUsed;
};

Ball balls[9];
int nballs=0;



typedef struct Box Box;

struct Box {
  double x,y,sz;
  int idx;
};

Box boxes[9];
int nboxes=0;



typedef struct Peg Peg;

struct Peg {
  double x,y,r;
  int idx;
};

Peg pegs[9];
int npegs=0;



int nused=0;



void SetMode(byte mode);
void VWait(void);
void Flip(void);

void ClearScreen(byte color);

void DrawPoint(int x,int y,byte color);
void DrawRect(int x,int y,int w,int h,byte color);
void FillRect(int x,int y,int w,int h,byte color);
void FillCircle(int x,int y,int r,byte color);
void DrawCircle(int x0,int y0,int radius,byte color);

void DrawChar(int font[],int x,int y,byte color,int ch);
void DrawText(int font[],int x,int y,byte color,const char *fmt,...);

char *GetRandLine(char *path);

char *substr(const char *str,int s,int l);

void addtoken(char ***tokens,int *ntokens,const char *token);
void tokenize(char ***token,int *ntokens,const char *del,const char *str);

void shuffleAnagrams(char **anagrams,int nanagrams);
char *shuffleWord(const char *w);

char *strupr(char *s);



int main(void) {

  char *line=NULL;

  char **anagrams=NULL;
  int nanagrams=0;

	int *guessed=NULL;
	int nguessed=0;

  char *word=NULL;
  char *shufword=NULL;

  int key;
  
  double diffx,diffy;
  
  double slow;
  
	time_t	current_time=0,
					start_time=0,
					end_time=0;
	
	double	elapsed_time=0,
					remaining_time=0,
					allotted_time=180.0;


	int points=0,vpoints=0;
	int score=0;

	bool gameover=false;

  int i,j,k,l;



  if (__djgpp_nearptr_enable() == 0) {
    printf("Could get access to first 640K of memory.\n");
    return 1;
  }

  VGA+=__djgpp_conventional_base;
  my_clock = (void *)my_clock + __djgpp_conventional_base;



  srand(time(NULL));                   /* seed the number generator. */



  line=GetRandLine(WORDLIST_FILE);
  tokenize(&anagrams,&nanagrams,",",line);

  word=strupr(strdup(anagrams[0]));
  shufword=shuffleWord(word);
  shuffleAnagrams(anagrams,nanagrams);

  printf("%s\n\n",shufword);

  printf("%s\n\n",word);

  for(i=0;i<nanagrams;i++) {
    if(i!=0) printf(", ");
    printf("%s",anagrams[i]);
  }
  printf("\n");

  getchar();



  SetMode(VGA_256_COLOR_MODE);       /* set the video mode. */



  outp(0x03C8,0);
  for(i=0;i<bicycle_num_colors;i++) {
    outp(0x03C9,(double)bicycle_palette[i*3+0]/255*63);
    outp(0x03C9,(double)bicycle_palette[i*3+1]/255*63);
    outp(0x03C9,(double)bicycle_palette[i*3+2]/255*63);
  }
  for(i=0;i<NUM_COLORS-bicycle_num_colors;i++) {
    outp(0x03C9,0);
    outp(0x03C9,0);
    outp(0x03C9,0);
  }  
  

  nballs=strlen(shufword);
  nboxes=nballs;
  npegs=nballs;
  for(i=0;i<nballs;i++) {
    balls[i].letter=shufword[i];
    balls[i].x=24*i+12;
    balls[i].y=34;
    balls[i].r=8;
    balls[i].dx=balls[i].x;
    balls[i].dy=balls[i].y;
    balls[i].isMoving=false;

    boxes[i].sz=balls[i].r*2+5;
    boxes[i].x=24*i+2+boxes[i].sz/2;
    boxes[i].y=2+boxes[i].sz/2;
		boxes[i].idx=-1;
		
		pegs[i].x=24*i+12;
    pegs[i].y=34;
    pegs[i].r=balls[i].r/2;
    pegs[i].idx=i;

  }

	guessed=calloc(nanagrams,sizeof(*guessed));

	slow=5;

	start_time=time(NULL);

  while(1) {

		current_time=time(NULL);

		elapsed_time=difftime(current_time,start_time);
		
		remaining_time=allotted_time-elapsed_time;

		if(remaining_time<0) remaining_time=0; 

    ClearScreen(2);

    for(i=0;i<nboxes;i++) {
      DrawRect(boxes[i].x-boxes[i].sz/2,boxes[i].y-boxes[i].sz/2,boxes[i].sz,boxes[i].sz,3);
    }
    
    for(i=0;i<npegs;i++) {
      FillCircle(pegs[i].x,pegs[i].y,pegs[i].r,2);
      DrawCircle(pegs[i].x,pegs[i].y,pegs[i].r,3);
    }

		for(i=0;i<nballs;i++) {
      FillCircle(balls[i].x,balls[i].y,8,2);
      DrawCircle(balls[i].x,balls[i].y,8,3);
      
      DrawChar(font,balls[i].x-3,balls[i].y-3,3,balls[i].letter);
    }

		DrawText(font,
				320-8*6-2,2,3,"%3d:%02d",
				(int)(remaining_time/60),
				(int)fmod(remaining_time,60));

    DrawText(font,320-8*7-2,12,3,"%07d",score);
    
    DrawText(font,320-8*7-2,22,3,"%3d/%3d",nguessed,nanagrams);
    
    
    if(points>0) { 
  		if(points-vpoints>=0) {
  			points-=vpoints;
	  		score+=vpoints; 
  		} else {
  			score+=points;
  			points=0;
  		}
  		vpoints++; 
    }

    VWait();
    Flip();

    delay(DELAY_MAX);

    if(kbhit()) {
      
      key=toupper(getch());

      if(key==VKEY_ESC) {
        break;
      } else if(key==VKEY_BSP) {
        if(nused>0) {
          k=-1;
          for(j=npegs-1;j>=0;j--) {
            if(pegs[j].idx==-1) {
              k=j;
              break;
            }
          }
          if(k!=-1) {
            nused--;
            balls[boxes[nused].idx].dx=pegs[k].x;
            balls[boxes[nused].idx].dy=pegs[k].y;
            balls[boxes[nused].idx].isUsed=false;
            balls[boxes[nused].idx].isMoving=true;
            pegs[k].idx=boxes[nused].idx;
            boxes[nused].idx=-1;
          }
        }
      } else if(key==VKEY_ENT) {
        char guess[10];
        
        for(i=0;i<nused;i++) {
          guess[i]=balls[boxes[i].idx].letter;
        }
        guess[i]='\0';
        
        l=-1;
        for(i=0;i<nanagrams;i++) {
          if(guessed[i]==0 && strcasecmp(guess,anagrams[i])==0) {
            l=i;
            break;
          }
        }
        
        if(l!=-1) {
          
          while(nused>0) {
            k=-1;
            for(j=npegs-1;j>=0;j--) {
              if(pegs[j].idx==-1) {
                k=j;
                break;
              }
            }
            if(k!=-1) {
              nused--;
              balls[boxes[nused].idx].dx=pegs[k].x;
              balls[boxes[nused].idx].dy=pegs[k].y;
              balls[boxes[nused].idx].isUsed=false;
              balls[boxes[nused].idx].isMoving=true;
              pegs[k].idx=boxes[nused].idx;
              boxes[nused].idx=-1;
            } else {
              break;
            }
          }
          
          vpoints=0;
          
          points+=strlen(anagrams[l])*100;
          
          guessed[l]=1;
          nguessed++;

					if(nguessed==nanagrams) {
						gameover=true;
					}

        }
        
      } else if(key==VKEY_SPC) {
        int tidx;
        
        for(i=npegs-1;i>0;i--) {
            j=rand()%(i+1);
    
            if(pegs[i].idx!=-1) {
              balls[pegs[i].idx].dx=pegs[j].x;
              balls[pegs[i].idx].dy=pegs[j].y;
              balls[pegs[i].idx].isMoving=true;
            }
            
            if(pegs[j].idx!=-1) {
              balls[pegs[j].idx].dx=pegs[i].x;
              balls[pegs[j].idx].dy=pegs[i].y;
              balls[pegs[j].idx].isMoving=true;
            }
            
            tidx=pegs[i].idx;
            pegs[i].idx=pegs[j].idx;
            pegs[j].idx=tidx;
        }
        
      } else if(key>='A' && key<='Z') {
        
        for(i=0;i<npegs;i++) {
          k=pegs[i].idx;
          if(k!=-1 && balls[k].letter==key) {
            pegs[i].idx=-1;
            boxes[nused].idx=k;
            balls[k].dx=boxes[nused].x;
            balls[k].dy=boxes[nused].y;
            balls[k].isMoving=true;
            balls[k].isUsed=true;
            nused++;
            break;
          }
        }
        
      }
     
    }

    for(i=0;i<nballs;i++) {
      if(balls[i].isMoving) {
        
        diffx=balls[i].dx-balls[i].x;
        diffy=balls[i].dy-balls[i].y;
        
        balls[i].x+=diffx/slow;
        balls[i].y+=diffy/slow;
        
        if(fabs(diffx)<=0.5) {
          balls[i].x=balls[i].dx;
        }
        
        if(fabs(diffy)<=0.5) {
          balls[i].y=balls[i].dy;
        }
        
        if(diffx==0 && diffy==0) {
          balls[i].isMoving=false;
        } 
        
      }
    }
    
  }

  SetMode(TEXT_MODE);

  __djgpp_nearptr_disable();

  return 0;
}



double drand() {
  return rand()/(RAND_MAX+1.0);
}



void SetMode(byte mode) {
  union REGS regs;

  regs.h.ah = SET_MODE;
  regs.h.al = mode;
  int86(VIDEO_INT, &regs, &regs);
}



void VWait(void) {
  /* wait until done with vertical retrace */
  while  ((inp(INPUT_STATUS) & VRETRACE)) {};
  /* wait until done refreshing */
  while (!(inp(INPUT_STATUS) & VRETRACE)) {};
}



void ClearScreen(byte color) {
  memset(dbuf,color,SCREEN_WIDTH*SCREEN_HEIGHT);
}



void Flip(void) {
  memcpy(VGA,dbuf,SCREEN_WIDTH*SCREEN_HEIGHT);
}



void DrawPoint(int x,int y,byte color) {
  if(x>=0 && x<SCREEN_WIDTH && y>=0 && y<SCREEN_HEIGHT) {
    dbuf[y*SCREEN_WIDTH+x]=color;
  }
}



void DrawRect(int x,int y,int w,int h,byte color) {
  int i,j;
  for(i=0;i<w;i++) {
    DrawPoint(x+i,y,color);
    DrawPoint(x+i,y+h-1,color);
  }
  for(j=0;j<h;j++) {
    DrawPoint(x,y+j,color);
    DrawPoint(x+w-1,y+j,color);
  }
}



void FillRect(int x,int y,int w,int h,byte color) {
  int i,j;
  for(j=0;j<h;j++) {
    for(i=0;i<w;i++) {
      DrawPoint(x+i,y+j,color);
    }
  }
}



void DrawCircle(int x0, int y0, int radius, byte color) {
  int f = 1 - radius;
  int ddF_x = 0;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;

  DrawPoint(x0, y0 + radius, color);
  DrawPoint(x0, y0 - radius, color);
  DrawPoint(x0 + radius, y0, color);
  DrawPoint(x0 - radius, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x + 1;
    DrawPoint(x0 + x, y0 + y, color);
    DrawPoint(x0 - x, y0 + y, color);
    DrawPoint(x0 + x, y0 - y, color);
    DrawPoint(x0 - x, y0 - y, color);
    DrawPoint(x0 + y, y0 + x, color);
    DrawPoint(x0 - y, y0 + x, color);
    DrawPoint(x0 + y, y0 - x, color);
    DrawPoint(x0 - y, y0 - x, color);
  }
}



void FillCircle(int x0,int y0,int r,byte color) {
  double x,y;
  double dx,dy;
  double d;
  
  for(y=0;y<r*2;y++) {
    for(x=0;x<r*2;x++) {
      dx=r-x;
      dy=r-y;
      d=sqrt(dx*dx+dy*dy);
  
      // Point lies outside of the circle
      if(d-r>1) continue;
  
      // Edge threshold
      if(r/d<1) continue;
      
      DrawPoint(x+x0-r,y+y0-r,color);
    }
  }

}
 


void DrawChar(int font[],int x,int y,byte color,int ch) {
  int i,j,k;
  for(j=0;j<8;j++) {
    for(i=0;i<8;i++) {
      k=font[64*ch+8*j+i];
      if(k==1) DrawPoint(x+i,y+j,color);
    }
  }
}



void DrawText(int font[],int x,int y,byte color,const char *fmt,...) {
  int i=x,j=y,k=0;

  va_list args;
  
  char *buf=NULL;
  int blen=0;
  
  va_start(args,fmt);
    blen=vsnprintf(NULL,0,fmt,args);
  va_end(args);
  
  buf=calloc(blen+1,sizeof(*buf));
  
  va_start(args,fmt);
    vsnprintf(buf,blen+1,fmt,args);
  va_end(args);
  
  while(buf[k]) {
    DrawChar(font,i,j,color,buf[k]);
    i+=8;
    if(i+8>SCREEN_WIDTH) {
      i=0;
      j+=8;
      if(j+8>SCREEN_HEIGHT) break;
    }
    k++;
  }
}



char *GetRandLine(char *path) {
  FILE *fin;
  char line[LINE_MAX];
  char sline[LINE_MAX];
  int n=0;
  char *p=NULL;
  fin=fopen(path,"rt");
  while(fgets(line,LINE_MAX,fin)!=NULL) {
    n++;
    if((double)1/n>drand()) {
      p=strchr(line,'\n');
      if(p) *p='\0';
      strcpy(sline,line);
    }
  }
  return strdup(sline);
}



char *substr(const char *str,int s,int l) {
  char *result=calloc(l+1,sizeof(*result));
  strncpy(result,str+s,l);
  return result;
}



void addtoken(char ***tokens,int *ntokens,const char *token) {
    (*tokens)=realloc(*tokens,sizeof(**tokens)*((*ntokens)+1));
    (*tokens)[(*ntokens)++]=strupr(strdup(token));
}



void tokenize(char ***tokens,int *ntokens,const char *del,const char *str) {
  const char *s=str;
  char *p=NULL;
  char *t=NULL;

  if(s==NULL) return;

  for(;;) {
    p=strstr(s,del);
    if(p==NULL) break;
    t=substr(s,0,p-s);
    addtoken(tokens,ntokens,t);
    free(t);
    t=NULL;
    s=p+strlen(del);
  }
  
  if(*s!='\0') {
    addtoken(tokens,ntokens,s);
  }

}



char *shuffleWord(const char *w) {
  char *r=strdup(w);
  char t;
  int i,j;

  for(i=strlen(r)-1;i>0;i--) {
    j=rand()%(i+1);
    t=r[i];
    r[i]=r[j];
    r[j]=t;
  }

  return r;
}



void shuffleAnagrams(char **anagrams,int nanagrams) {
  int i,j;
  char *t=NULL;
  for(i=nanagrams-1;i>0;i--) {
    j=rand()%(i+1);
    t=anagrams[i];
    anagrams[i]=anagrams[j];
    anagrams[j]=t;    
  }
}



char *strupr(char *s) {
  int i=0;
  while(s[i]) {
    s[i]=toupper(s[i]);
    i++;
  }
  return s;
}


