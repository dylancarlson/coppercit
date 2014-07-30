/************************************************************************/
/*  Borland C++ 2.0  8/92                                  */
/*  bds1.46  9/89                      modem.c           */
/*    modem code for Citadel bulletin board system    */
/* NB: this code is rather machine-dependent:  it will typically  */
/* need some twiddling for each new installation.        */
/*             82Nov05 CrT          */
/************************************************************************/

/************************************************************************/
/*          history           */
/*                         */
/* 92Aug05 JCL Ported to MSDOS                         */
/* 91Aug04 JCL Dumped puter.c added that code here    */
/* 88Apr21 BP   Dump interp, add accounting, all in c, speed up         */
/* 86Apr23 BP,KK,MM networks, print with carr, inVis(), etc    */
/* 83Mar01 CrT FastIn() ignores LFs etc -- CRLF folks won't be trapped.*/
/* 83Feb25 CrT Possible fix for backspace-in-message-entry problem.  */
/* 83Feb18 CrT fastIn() upload mode cutting in on people.  Fixed. */
/* 82Dec16 dvm modemInit revised for FDC-1, with kludge for use with */
/*    Big Board development system           */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov15 CrT readfile() & sendfile() borrowed from TelEdit.c    */
/* 82Nov05 CrT Individual history file established       */
/************************************************************************/

#include "ctdl.h"
#include <bios.h>

#define WCSECTSIZE 128   /* size of Xmodem block */

char getCh();
char modIn();
char carrDet();
char visible();
char mIready();
char mOready();
char hangUp();
char getMod();
/* static int blank_flag, bl_ok; */
/************************************************************************/
/*          Contents          */
/*                         */
/* BBSCharReady()    returns true if user input is ready */
/*  # fastIn()    kludge code compiling other stuff inline*/
/* getCh()     bottom-level console-input ra.filter   */
/*  # getMod()    bottom-level modem-input   ra.filter   */
/* interpret()    interprets a configuration routine  */
/* KBReady()      returns TRUE if a console char is ready */
/* getCh()     returns a console char        */
/* iChar()     top-level user-input function    */
/*      inVis()         kk's ra.filter for nets       */
/* modIn()     returns a user char        */
/* mOReady()      returns true if modem can accept a char */
/* oChar()     top-level user-output function      */
/*  # outMod()    bottom-level modem output     */
/* pause()     pauses for N/36 seconds      */
/* putChar()                     */
/* putFlChar()    readFile() -> disk file interface   */
/* putWCChar()    ra.filter[]s, readFile() to putMsgChar()  */
/* receive()      read modem char or time out      */
/* transmitFile()    send a host file, no formatting  */
/* visible()      convert control chars to letters */
/*                         */
/* # == routines you should certainly check when porting system   */
/************************************************************************/

/************************************************************************/
/*    The principal dependencies:            */
/*                         */
/*  iChar   modIn              outMod     */
/*     modIn   getMod  getCh   mIReady kBReady outMod  carrDetect */
/*        getMod                 */
/*           getCh               */
/*              mIReady          */
/*                 kBReady       */
/*                       carrDetect */
/*                         */
/*  oChar                   outMod     */
/*                    outMod  mOReady  */
/************************************************************************/


/************************************************************************/
/* BBSCharReady() returns TRUE if char is available from user  */
/* NB: user may be on modem, or may be sysop in CONSOLE mode   */
/************************************************************************/
char BBSCharReady()
{
char KBReady();

return ( (ra.haveCarrier  &&  /**interpret(pMIReady)*/mIready() )
   || (ra.whichIO==CONSOLE  &&   KBReady() )
   || (KBReady() && getCh() == SPECIAL)
   );

}

/************************************************************************/
/* getCh() reads a console char              */
/*     In CONSOLE mode, CRs are changed to newlines      */
/*     Rubouts are changed to backspaces           */
/* Returns: resulting char             */
/************************************************************************/
char getCh()
{
 char c;
 
 c = bioskey(0);

if(c == CNTRLp) {
     doStatusLine(S_MSGS, "CNTRL-P is Disabled");
     return(' ');
}     
else
   if( c == CNTRLa)
     { 
       printer_toggle();
       return(' ');
     }  
   else
      return (c & 0x7F);                  
/*return (char) (getch() & 0x7F);*/
}

/************************************************************************/
/* getMod() is bottom-level modem-input routine       */
/*   kills any parity bit                 */
/*   rubout       -> backspace         */
/*   CR           -> newline        */
/*   other nonprinting chars  -> blank       */
/* Returns: result                  */
/************************************************************************/
char getMod() {
extern int int86();

union REGS regs;
regs.x.ax = 0x0200;   /* get char from fossil buffer */
regs.x.dx = ra.portno;   /* for this port */
int86(0x14, &regs, &regs);
return (regs.h.al);
}
/************************************************************************/
/* getMod_Peek() is bottom-level modem-input look-ahead routine       */
/*   kills any parity bit                 */
/*   rubout       -> backspace         */
/*   CR           -> newline        */
/*   other nonprinting chars  -> blank       */
/* Returns: result                  */
/************************************************************************/
char getMod_Peek() {
extern int int86();

union REGS regs;
regs.x.ax = 0x0C00;   /* get char from fossil buffer */
regs.x.dx = ra.portno;   /* for this port */
int86(0x14, &regs, &regs);
return (regs.h.al);
}


/************************************************************************/
/* iChar() is the top-level user-input function -- this is the */
/* function the rest of Citadel uses to obtain user input      */
/************************************************************************/

     
char iChar() {
char modIn();
char c;

scrn = 0;

if (ra.justLostCarrier)   return 0;   /* ugly patch  */

blank_flag = FALSE;

c = ra.filter[modIn()];


if (c==SPECIAL) c=' ';

/* if(ra.textDownload) return(c);*/

switch (ra.echo) {
case BOTH:
   if (ra.haveCarrier) {
      if (c == '\n') doCR();
      else    outMod(c);
      }
   if (c == '\n') cprintf ("\n\r");
      else putch(c);
   break;
case CALLER:
   if (ra.whichIO == MODEM) {
      if (c == '\n')   doCR();
      else    outMod(c);
      }    
   else {
      if (c == '\n') cprintf("\n\r");
      else putch(c);
      }
   break;
case ASTERISK:   
   if (ra.haveCarrier) {
      if (c == '\n')   doCR();
      else    outMod((c=='\b' ? '\b' : '*'));
      }
   if (c == '\n') cprintf ("\n\r");
      else putch((c == '\b' ? '\b' : '*'));
   break;

}
return(c);
}


/************************************************************************/
/* KBReady() returns TRUE if a console char is ready     */
/************************************************************************/
char KBReady()
   {
   return (char) kbhit();
/*    return (char) _bios_keybrd(_KEYBRD_READY);*/
   }

/************************************************************************/
/* modIn() toplevel modem-input function           */
/*   If DCD status has changed since the last access, reports     */
/*   carrier present or absent and sets flags as appropriate.     */
/*   In case of a carrier loss, waits 20 ticks and rechecks    */
/*   carrier to make sure it was not a temporary glitch.    */
/*   If carrier is newly received, returns ra.newCarrier = TRUE;  if */
/*   carrier lost returns 0.  If carrier is present and state     */
/*   has not changed, gets a character if present and       */
/*   returns it.  If a character is typed at the console,      */
/*   checks to see if it is keyboard interrupt character.  If     */
/*   so, prints short-form console menu and awaits next     */
/*   keyboard character.                  */
/* Globals modified: carrierDetect  ra.modStat  ra.haveCarrier */
/*       ra.justLostCarrier ra.whichIO    ra.exitToCpm   */
/*       visibleMode             */
/* Returns: modem or console input character,         */
/*    or above special values             */
/* 1984 Kerry Kyes & Ed Clark add baud detect patch      */
/* 1987 .bp. uses saccit's baud detect in c         */
/************************************************************************/
char modIn() {
   char getMod(), getCh(), KBReady();
   char i, s[4],  ch;
   unsigned    hi, lo, zz, c=0;
   void get_baud();
   time_t hh;
      
   hi = 20; /*(HITIMEOUT * ra.megaHz);*/ /* about 4 ra.minutes  */
   lo = 0xFF;

   startTimer();
   hh = time(NULL);
   while (TRUE) {


       fix_rollover();

      if ((ra.whichIO==MODEM) && (c=carrDet()) != ra.modStat) {
    /* carrier changed   */
    if (c)  {     /* carrier present   */
       doStatusLine(S_MSGS, "Carrier Detect");
       ra.haveCarrier = TRUE;
       pause(40);
       ra.guess = 3;
       ra.modStat     = c;
       ra.newCarrier  = TRUE;
       ra.rtry      = 5;
       getDaTime();
       ra.lhour = ra.hour;
       ra.lminute = ra.minute;
       return(0);
       }
    else {
       sleep(1);        /* confirm it's not a glitch */
       if (!carrDet()) {   /* check again */
     doStatusLine(S_MSGS, "Carrier Loss");
     ra.haveCarrier     = FALSE;
     ra.modStat     = FALSE;
     ra.justLostCarrier = TRUE;
     terminate(TRUE);
/*     hangUp();*/
     return(0);
     }
       }
    }
      if (mIready()) {
    if (ra.haveCarrier) {
       c = (getMod() & 0x7F);
       if (ra.whichIO == MODEM)   return(c);
       }       
    else {       /* no carrier, so it must be  */
       if (ra.whichIO == MODEM) {  /* a modem result code.    */
          get_baud();
          if (ra.baud == -1) /* return 0;*/ {
             if (ra.lockedport && carrDet() /* ra.haveCarrier*/ ) 
                ra.baud = 2400; /* make believe it's 2400 */
             else return 0;
             }   
          if (ra.baud < RING && ra.baud > -1) { 
            neverLoggedIn = TRUE;
            getDaTime();

            sprintf(format, "%2d|%02d|%02d %02d:%02d ",
                    ra.year,
                    ra.month,
                    ra.date,
                    ra.hour,
                    ra.minute         );
           sprintf(&ra.call_record.time_day,"%-18s", format);
         }
          timeout = time(NULL) + ra.timeout/4;
       }
       }
    }

      if (KBReady()) {
    c = bioskey(0); /* getCh(); */
    if(!(c & 0xFF)) {
       /* must be a special key */
       switch(c >> 8){
         case F10 :
            for_sysop = TRUE;
            for ( zz = 0; zz < 7; zz++) {
              delay(500);
              oChar(BELL);
            }  
            doCR();
            doCR();
            tutorial("warning.blb");
            doCR();
            doCR();
            break;
         case F9  :
            mPrintf("%^&*(&^GYFGR^&{{}}{{{}}{{{}}}|+*(^}&^");
            byeflag = FALSE;
            if (carrDet() )terminate(TRUE);
            break;
         }
         return(0);
     }
    ch = (char) c;            
    if (ra.whichIO == CONSOLE) {
     if (ch != SPECIAL) return(ch);
     else { ra.whichIO = MODEM;
       timeout = time(NULL) + (ra.loggedIn ? ra.timeout : ra.timeout/4);
       doStatusLine(WHICH_IO, "MODEM  ");
       if (!carrDet()) doOnhook();
       if(!ra.haveCarrier)doStatusLine(S_MSGS, "Waiting...");
       return(0);
          }
     }
    else {
       if (ch == SPECIAL) {
     doStatusLine(WHICH_IO, "console");
     doStatusLine(S_MSGS,   "                         ");
     ra.whichIO = CONSOLE;
     if (!ra.loggedIn) setUp(FALSE);
     if (!carrDet()) doOffhook();
     timeout = time(NULL) + (ra.loggedIn ? ra.timeout : ra.timeout/4);
     return(0);
     }
       }
    }

    /* check for no input.  (Short-circuit evaluation, remember!) */
    if ( (ra.whichIO==MODEM  &&  ra.haveCarrier) || ra.onConsole ) {
       if( (time(NULL)) > timeout) {
       terminate(TRUE);
       return(0);
       }
   }
   
    } /*while TRUE */
}  /*finished*/

/************************************************************************/
/* oChar() is the top-level user-output function         */
/*   sends to modem port and console both          */
/*   does conversion to upper-case etc as necessary      */
/*   in "ra.debug" mode, converts control chars to uppercase letters */
/* Globals modified: ra.prevChar          */
/************************************************************************/
oChar(c)
char c;
{
ra.prevChar = c;       /* for end-of-paragraph code  */
if (ra.outFlag) return;      /* s(kip) mode       */

/* if (ra.termUpper)   c = toupper(c);*/
if (ra.debug)    c = visible(c);
if (c == NEWLINE)   c = ' '; /* doCR() handles real newlines */

/* show on console         */
/*    if (visibleMode || ra.whichIO==CONSOLE || (ra.thisRoom!=1 && ra.echo!=CALLER))
*/    putch(c);


if (p_port && !ra.usingWCprotocol)
  if( c > 32 && c < 127)
    if(biosprint(0, c, p_port-1) & 0x29) 
      doStatusLine(S_MSGS, "\07 Check Printer \07 \n\r");



if (ra.haveCarrier && c != SPECIAL)  {
   if (!ra.usingWCprotocol) {
      if(ra.outFlag == ASTERISK) outMod('*');
      else outMod(c);       /* show on modem     */
      }    
   } 

return TRUE;
}


/************************************************************************/
/* outMod stuffs a char out the modem port         */
/************************************************************************/
outMod(c)
   char c;
{
extern int int86();
union REGS regs;

/* delay between each character send if BBS is on a locked port
   to prevent modem buffering from not reacting quickly to a user 'P' or 'S'.
   we round up the delay by using one less charcter per second than the
   baud rate actually sends, e.g. 300 baud=30 chars so the time to send
   one character is 1000/30 <in milliseconds> so we divide 1000 by 29, etc */
   
/* int dlay[] = { 1000/29, 1000/119, 1000/239, 1000/479, 1000/960}; */

/* ho-hum, hang around and wait to send this little varmint */
while(!/**interpret(pMOReady)*/mOready());

if (ra.debug) putch(c);

regs.h.ah=0x0B;  /* send character */
regs.h.al=c;     /* and here is the character to send*/
regs.x.dx=ra.portno;  /* and here is where to send it */
int86(0x14,&regs,&regs);  /* adios */


if(ra.loggedIn && !ra.nopace &&
   !ra.usingWCprotocol && ra.whichIO == MODEM && carrDet() ){
   delay(ra.baud >= 9600 ? 1 : 10000/ra.baud);
}
return TRUE;
}
/************************************************************************/
/* putChar()                     */
/************************************************************************/
putChar(c)
char c;
{
    putch(c);
    return TRUE;
}

/************************************************************************/
/* putFlChar() is used to upload files          */
/* returns: ERROR on problems else TRUE            */
/* Globals: ra.msgBuf.mbtext is used as a buffer, to save space   */
/*      modified 2/21/87 to return error if putC() works   .bp.      */
/************************************************************************/
putFlChar(c)
char c;
{
char a;
a=c;
return  (char) _write( upld, &a, 1)  /* != ERROR */ ;
}

/************************************************************************/
/* putWCChar() ra.filter[]s from file pointed to by upld       */
/* Returns: ERROR if problems, else TRUE           */
/* Globals: ra.outFlag is set to OUTSKIP when an  EOF  is   */
/*     encountered, and no further text is written to disk. */
/************************************************************************/
int putWCChar()

{
char c;
/*if (ra.outFlag)  return TRUE; */  /* past ascii EOF.  ignore.   */

close(upld);

upld = open(pc, O_RDWR | O_BINARY);

lseek(upld, 0, 0);

while(_read(upld, &c, 1) == 1){
   c = ra.filter[c];
   if (c!=0 && c!=SPECIAL) putMsgChar(c);
   }
return TRUE;
}


/************************************************************************/
/* transmitFile() dumps a host file with no formatting      */
/************************************************************************/
transmitFile(filename)
   char *filename;
{
char mAbort();
char /*fbuf[BUFSIZ],*/ fname[NAMESIZE];
int  c, new_line;
FILE *fp;
extern int fgetc();

ra.outFlag = OUTOK;

unspace(filename, fname);

if((fp = fopen(fname, "r" /*fbuf*/)) == NULL) {
   ra.outFlag = OUTSKIP; /*these 2 lines seem to be ignored */
   fclose(fp /*fbuf*/);
   return(ERROR);
   }

if(!ra.usingWCprotocol) doCR();
if(ra.ansi_on) cprintf("\n\r Transmitting %s \n\r", filename);
new_line = 1;
c=fgetc(fp);
while ( (c != EOF /*ERROR*/) && !mAbort() && ( (ra.whichIO == CONSOLE)
   ||
   (carrDet()) ) )
   {
   if (new_line == 3 && c < 33 ) {
      outMod('\b');
      outMod('\b');
      doCharPause(1);
/*      while((c=fgetc(fp)) != EOF && c != NEWLINE);  */
      new_line = 1;
   }
   else {   
   
   if (new_line == 1 && c == '@') new_line = 2;
      else if(new_line == 2 && c == 'P' ) {
           outMod('\b');
           if (!ra.ansi_on && !ra.termMore) doCharPause(1);
           else { doCharPause(0);
                  modIn();
                  }
           c = '\0';
           }
           
   if (!ra.usingWCprotocol) {
      if ( c != NEWLINE ) {
         if(!ra.ansi_on) c = ra.filter[c];
         if ((c != SPECIAL) && !ra.ansi_on && (c != 0)) putch(c);
      }   
      else {
         doCR();
         new_line = 1;
     }    
   }   
   /* keep off the screen */
   if (ra.haveCarrier) 
      if (!ra.usingWCprotocol)
        if ( (c != NEWLINE) && (c != 0))  outMod(c);
   }     
   if (ra.textDownload  &&  c==CPMEOF)   break;
   if (ra.textDownload  &&  mAbort() )   break;
   c=fgetc(fp);
   }

fclose(fp /*fBuf*/);

return TRUE;
}
/************************************************************************/
/* transmitFILEINFO() dumps FILEINFO with no formatting      */
/************************************************************************/
transmitFILEINFO()
   
{
char mAbort();
char fbuf[128], ch, flag, room_[14];
FILE *fp;

ra.outFlag = OUTOK;

sprintf(room_, "FILEINFO.%03d", ra.thisRoom);

setSpace(ra.homeDisk, ra.homeUser);
chdir("FILEINFO");

if((fp = fopen(room_, "r" /*fbuf*/)) == NULL) {
   ra.outFlag = OUTSKIP; /*these 2 lines seem to be ignored */
   fclose(fp /*fbuf*/);
   return(ERROR);
   }

fseek(fp, 0, 0);

doCR();

flag = 1;
mPrintf("\n Scroll On What Letter <CR For All>: ");
ch = (char) toupper( iChar() );
if ( ch == '\r' || ch == '\n') flag = 0;

doCR();

while(fgets(fbuf, 128, fp) != NULL) 
  if((flag && toupper(fbuf[0]) == ch) || !flag) {
     mPrintf(fbuf);
     doCR();
  }
     

  
fclose(fp /*fBuf*/);

setSpace(ra.homeDisk, ra.homeUser);

return TRUE;
}

/************************************************************************/
/* visible() converts given char to printable form if nonprinting */
/************************************************************************/
char visible(c)
char c;
{
char a;
if ( c > 31 && c < 127) return(c);
a = c;
if (c==0xFF)  c = '|'  ;   /* start-of-message in message.buf */
c        = c & 0x7F ;   /* kill high bit otherwise      */
if (c== 0x7F) c = 'D'  ;   /* catch DELETE too       */
if ( c < ' ') c = c + 'A' -1;   /* make all control chars letters   */
putch(c);
return(a);
}

/*********************************************************************/
/*                                                                   */
/*   STUFF THAT WAS IN PUTER.C WHICH REPLACED THE INTERPRETER        */
/*                                                                   */
/*********************************************************************/

getDaTime()

{
struct date d;
struct time t;
struct tm *tb;
time_t timer;

fix_rollover();

if (!ra.AT) {
   getdate(&d);
   gettime(&t);

   ra.year = d.da_year;

   if(ra.year < 2000) ra.year -= 1900;
   else ra.year -= 2000;

   ra.month = d.da_mon;
   ra.date = d.da_day;
   first_flag = 0;
   
   ra.hour = t.ti_hour;
   ra.minute = t.ti_min;
   }
else
      
#if 0
union REGS regs;

regs.h.ah = 0x02;  /* get time */
regs.h.al = 0;
regs.x.dx = regs.x.cx = regs.x.bx = 0;
int86(0x1A,&regs,&regs);

ra.hour = (10*(regs.h.ch >> 4)) + (regs.h.ch & 0x0F);
ra.minute = (10*(regs.h.cl >> 4)) + (regs.h.cl & 0x0F);

regs.h.ah = 0x04;  /* get date */
regs.h.al = 0;
regs.x.dx = regs.x.cx = regs.x.bx = 0;
int86(0x1A,&regs,&regs);

ra.year = (10*( regs.h.cl >> 4)) + (regs.h.cl & 0x0F);
ra.month = (10*( regs.h.dh >> 4)) + (regs.h.dh & 0x0F);
ra.date = (10*(regs.h.dl >> 4)) + (regs.h.dl & 0x0F);
#endif
   {

   timer = time(NULL);
   tb = localtime(&timer);

   ra.year = tb->tm_year;
   ra.month = tb->tm_mon + 1;
   ra.date = tb->tm_mday;
   first_flag = 0;
   
   ra.hour = tb->tm_hour;
   ra.minute = tb->tm_min;
  }
return TRUE;

}


/************************************************************************/
/* 11/87    timeOn                  */
/*       for time online               */
/************************************************************************/
int timeOn()
{
char baro;
int mOn, hOn;

getDaTime();
if(baro=(ra.lminute > ra.minute)){
   mOn = ra.minute + 60 - ra.lminute;
   }
else mOn = ra.minute - ra.lminute;

   if (ra.lhour > ra.hour) hOn = 24 - ra.lhour + ra.hour;
   else     hOn = ra.hour - ra.lhour;

   if (baro) {
      --hOn;
      baro=FALSE;
      }

mOn = mOn + (60 * hOn);
if (ra.twit && carrDet()) mOn = 3 * mOn;
return (mOn);
}

/*

====> Note Well: This version of modem routines (including the modem
      in and out routines above require a FOSSIL to be in place. These
      routines should work with either X00 or BNU.
      
*/

/***********************************************************************/
/* 8/91     initPort                                                   */
/*          set up fossil for selected port                            */
/***********************************************************************/
initPort()

{ 
extern int int86();
union REGS regs;
int i;

regs.h.ah = 4; /* initialize fossil */
regs.x.dx = ra.portno;  /* designate COMx */

if( int86(0x14,&regs,&regs) != 0x1954)  /* fossil id not sent back */
   {
   cprintf(" \x07 Port Number Error or Fossil Not Found \n\r");
   cprintf(" Make Sure Fossil Is Installed \n\r \x07");
   cprintf(" Exiting Program - ERRORLEVEL = 195 \n\r");
   rename("ctdltabl.$$$","ctdltabl.sys");
   setdisk(2);
   exit(195);
   }

sleep(1);

if (ra.hibaud > 9600) ra.lockedport = TRUE;

if(ra.lockedport)
   {
   /* setup hardware flow control */
   regs.x.ax = 0x0F02; /* rts/cts flow control */
   regs.x.dx = ra.portno;
   int86(0x14,&regs,&regs);
   doStatusLine(S_MSGS, "Locked port-rts/cts flow");
   }
else
  doStatusLine(S_MSGS, "Init Port");

/* if(!ra.lockedport) */
   setbaud(ra.hibaud);

/* purge  input buffer  */
regs.x.ax = 0x0A00; 
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);


regs.x.ax = 0x0900; /* purge output buffer  */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

i = 0;

if(!no_cit_init && !is_mailer) {
  initString();
  delay(100);
  
}

return TRUE;
}

/************************************************************
 flush_fossil()
 ***********************************************************/
 
 flush_fossil()
 {
 extern int int86();
 union REGS regs;

  regs.x.ax = 0x0800; /* flush output buffer  */
  regs.x.dx = ra.portno;
  int86(0x14,&regs,&regs);

}

/************************************************************
 
 deinit_fossil()  - removes FOSSIL from controlling port
 
************************************************************/

void deinit_fossil() 
{
extern int int86();
union REGS regs;

regs.h.ah = 5; /* de-initialize fossil */
regs.x.dx = ra.portno;  /* designate COMx */

int86(0x14,&regs,&regs);

}

/************************************************************/
/*  8/91                                                    */
/*       the following set baud rates then calls            */
/*       routine setbaud() that actaully sets the port      */
/************************************************************/

set300()
{
setbaud(0);
}

set1200()
{
setbaud(1);
}

set2400()
{
setbaud(2);
}

set4800()
{
setbaud(3);
}

set9600()
{
setbaud(4);
}

set19200()
{
setbaud(5);
}

set38400()
{
setbaud(6);
}

setbaud(baud)
unsigned int baud;

{
extern int int86();

union REGS regs;

switch(baud)
{
case 300: 
   regs.x.ax = 0x0043; 
   break;
case 1200: 
   regs.x.ax = 0x0083; 
   break;
case 2400: 
   regs.x.ax = 0x00A3; 
   break;
case 4800: 
   regs.x.ax = 0x00C3; 
   break;
case 9600: 
   regs.x.ax = 0x00E3; 
   break;
case 19200: 
   regs.x.ax = 0x0003; 
   break;
case 38400: 
   regs.x.ax = 0x0023; 
   break;

default: 
   doStatusLine(S_MSGS, "\x07 Baud Rate Error in SETBAUD routine \x07 ");
}

regs.x.dx = ra.portno;
int86(0x14, &regs, &regs);
return TRUE;
}

/*******************************************************************/
/* 8/91                                                            */
/*      mIready     -  char waiting from modem query               */
/*******************************************************************/
char mIready()

{
union REGS regs;

regs.x.ax = 0x0300;  /* request serial port status */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

return((regs.h.ah & 0x80) ? 0 : (regs.h.ah & 0x01)); 

}

/*******************************************************************/
/* 8/91                                                            */
/*      mOready     -  transmit buffer space available query       */
/*******************************************************************/
char mOready()

{
union REGS regs;

regs.x.ax = 0x0300;  /* request serial port status */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

return((regs.h.ah & 0x80) ? 0 : (regs.h.ah & 0x20));

}

/*******************************************************************/
/* 8/91                                                            */
/*      carrDet     -  carrier detect query                        */
/*******************************************************************/
char carrDet()

{
union REGS regs;

regs.x.ax = 0x0300;  /* request serial port status */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

return((regs.h.ah & 0x80) ? 0 : (regs.h.al & 0x80));

}

/*********************************************

pull_DTR yanks DTR but doesn't re-init modem

**********************************************/

pull_DTR()

{
union REGS regs;

regs.x.ax = 0x0600;  /* drop DTR */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

delay(500);  /*about 1 second*/

regs.x.ax = 0x0601;  /* raise DTR */
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);

delay(2000);

/* hard as it may seem to believe, some modems actually regurgitate
    characters after a hardware reset of DTR, so lets clean the buffer */
/* purge  input buffer  */
/*regs.x.ax = 0x0A00; 
regs.x.dx = ra.portno;
int86(0x14,&regs,&regs);
*/
}


/*****************************************************************/
/* 8/91                                                          */
/*       hangUp   -  by way of yanking DTR                       */
/*****************************************************************/

char hangUp()

{

pull_DTR();

if(!is_mailer)
  initPort(); /* in case a cheap modem forgets what we commanded earlier */
else
  deinit_fossil();
  
return TRUE;
}

/*********************************************************************
*                                                                    *
* printer_toggle() - toggles output to printer                       *
*                                                                    *
*********************************************************************/

printer_toggle()

{
  int try, bottom, top;
  char numstring[NAMESIZE];

   bottom = 0;
   top = 2;


  cprintf("\r\n Hard Copy of Session ");


   do {
   cprintf("\n\r 0 - Abort, 1 - LPT1:, 2 - LPT2 : ");
   gets(numstring);
   try   = atoi(numstring);
   if (try < bottom)
      {  
      sprintf(format, ">= %d\n\r ", bottom);
      cprintf("%s", format);
      }
   if (try > top  ) 
      {
      sprintf(format, "<= %d\n\r ", top);
      cprintf("%s",format);
      }
   } 
while (try < bottom ||  try > top);

p_port =  try;
if(p_port){
  if(biosprint(1, 0, p_port-1) & 0x39){
    cprintf("\n\rPrinter Port Error, not Initialized\n\r");
    p_port = 0;
    }  
  else
    cprintf("\n\rPrinter Port LPT%d Initialized\n\r", p_port);
  }
}


/************************************************************************/
/* TIMERS:                                                              */
/*    Basically, the idea here is that two functions are available to   */
/* the rest of Citadel.  One starts a timer.  The other allows checking */
/* that timer, to see how much time has passed since that timer was     */
/* started.  The remainder of the functions in this section are internal*/
/* to this implementation, mostly for use by receive().                 */
/************************************************************************/

static struct timePacket localTimer;

/************************************************************************/
/*      chkTimeSince() buffer for timing stuff.  A call to startTimer() */
/*      must have preceded calls to this function.                      */
/*      RETURNS: Time in seconds since last call to startTimer().       */
/************************************************************************/
long chkTimeSince()
{
    long timeSince();

    return timeSince(&localTimer);
}

/************************************************************************/
/*      milliTimeSince() Calculate how many milliseconds have passed    */
/************************************************************************/
long milliTimeSince(Slast)
struct timePacket *Slast;
{
    long retVal;
    struct time timeblk;

    gettime(&timeblk);
    retVal = (timeblk.ti_sec != Slast->tPsecond) ? 100 : 0;
    retVal += timeblk.ti_hund - Slast->tPmilli;

    return retVal;
}

/************************************************************************/
/*      pause() busy-waits N/100 seconds                                */
/************************************************************************/
pause(i)
int i;
{
/*    struct timePacket x;
    long timeSince(), milliTimeSince(), (*fn)(), limit;

    
    fn = (i <= 99) ? milliTimeSince : timeSince;
    limit = (i <= 99) ? (long) i : (long) (i / 100);    /* Kludge */
    setTimer(&x);
    while ((*fn)(&x) <= limit)
        ;
*/

delay(i);

}

/************************************************************************/
/*      setTimer() get ready for timing something                       */
/************************************************************************/
setTimer(Slast)
struct timePacket *Slast;
{
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);

    Slast->tPday     = (long) dateblk.da_day;
    Slast->tPhour    = (long) timeblk.ti_hour;
    Slast->tPminute  = (long) timeblk.ti_min;
    Slast->tPsecond  = (long) timeblk.ti_sec;
    Slast->tPmilli   = (long) timeblk.ti_hund;
}

/************************************************************************/
/*      startTimer() Initialize a general timer                         */
/************************************************************************/
startTimer()
{
    setTimer(&localTimer);
}

/************************************************************************/
/*      timeSince() Calculate how many seconds have passed since "x"    */
/************************************************************************/
static long timeSince(Slast)
struct timePacket *Slast;
{
    long retVal;
    struct date dateblk;
    struct time timeblk;

    getdate(&dateblk);
    gettime(&timeblk);

    retVal = (Slast->tPday == dateblk.da_day ? 0l : 86400l);
    retVal += ((timeblk.ti_hour - Slast->tPhour) * 3600);
    retVal += ((timeblk.ti_min - Slast->tPminute) * 60);
    retVal += (timeblk.ti_sec - Slast->tPsecond);
    return retVal;
}

/************************************************************************/
/*      receive() gets a modem character, or times out ...              */
/*      Returns:        char on success else ERROR                      */
/************************************************************************/

int receive(seconds)
int seconds;

{
 clock_t t;
 
 seconds *= 18;  /*tic factor*/
 
 t = clock();
 
 t += (clock_t) seconds; /* limit to wait */
 
 do{
   if (mIready() ) return((int) getMod() );
 }
 while( (t > clock()) && (carrDet() || !ra.haveCarrier));
 
 return(ERROR);

}
    
/*******************************************************

purge_input_buffer()  clears all chars from input buffer

********************************************************/

purge_input_buffer()
{
  union REGS regs;

  /* while (mIready()) iChar();  */
  /* purge  input buffer  */
  regs.x.ax = 0x0A00;
  regs.x.dx = ra.portno;
  int86(0x14,&regs,&regs);
}


doOffhook()
{
 int i;
 

 i=0;
 while (ra.off_hook[i] != '\0')  outMod(ra.off_hook[i++]);

}

doOnhook()
{
 int i;
 int c;

 if (ra.attn[0] == '|')  c = atoi(&ra.attn[1]);
 else c = (int) ra.attn[0];

 i=0;
 if (carrDet() ) {
    for (i = 0; i++; i < 3) outMod( (char) c);
    }
 sleep(3);     
 i=0;
 while (ra.on_hook[i] != '\0')  outMod(ra.on_hook[i++]);
 outMod('\r');

}
/*************************************************
initString()  sends init strings to modem
*************************************************/

initString()
{
  int i;
  char *str = "ATX4V1\r";
  
  i=0;
  while(ra.sModStr[i] != '\0') outMod(ra.sModStr[i++]);
  outMod('\r'); /* just in case sysop forgot newline */
  delay(500);
  i=0;
  while(ra.init2[i] != '\0') outMod(ra.init2[i++]);    
  outMod('\r'); /* and once again */
  delay(500);
  i=0;
  while(*str != '\0') outMod(*str++);

}
/****************************************************
doAnswerPhone()  takes the phone off-hook at a RING
****************************************************/

doAnswerPhone()
{
 int i;
 

 i=0;
 while (ra.answer_phone[i] != '\0')  outMod(ra.answer_phone[i++]);

}
