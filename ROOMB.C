/************************************************************************/
/* Borland C++ 2.0 8/92                      */
/* bds1.46  9/89        roomb.c        */
/*    room code for Citadel bulletin board system     */
/************************************************************************/

/************************************************************************/
/*          History           */
/*                         */
/* 92Aug05 JCL Ported to MSDOS                      */
/* 87Jan28 BP   hidden room fix, insert, etc          */
/* 86Apr24 BP,MM,KK formating                */
/* 83Feb26 CrT bug in makeRoom when out of rooms fixed.     */
/* 83Feb26 CrT matchString made caseless, normalizeString()    */
/* 83Feb26 CrT "]" directory prompt, user name before prompts     */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov02 CrT Cleanup prior to V1.2 mods.            */
/* 82Nov01 CrT Proofread for CUG distribution.        */
/* 82Mar27 dvm conversion to v. 1.4 begun          */
/* 82Mar25 dvm conversion for TRS-80/Omikron test started      */
/* 81Dec21 CrT Log file...                */
/* 81Dec20 CrT Messages...                */
/* 81Dec19 CrT Rooms seem to be working...            */
/* 81Dec12 CrT Started.                */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*          Contents          */
/*                         */
/* editText()     handles the end-of-message-entry menu  */
/* findRoom()     find a free room        */
/* getNumber()    prompt user for a number, limited range */
/* getRoom()      load given room into RAM      */
/* getString()    read a string in from user    */
/* getText()      reads a message in from user     */
/* getYesNo()     prompts for a yes/no response    */
/* givePrompt()      gives usual "THISROOM>" prompt      */
/* indexRooms()      build RAM index to ctdlroom.sys  */
/* interact()     chat mode            */
/* makeRoom()     make new room via user dialogue  */
/* matchString()     search for given string       */
/* modemInit()    top-level initialize-all-modem-stuff   */
/* normalizeString() cleans it up            */
/* noteRoom()     enter room into RAM index     */
/* putRoom()      store room to given disk slot    */
/* renameRoom()      sysop special to rename rooms    */
/* replaceString()   string-substitute for message entry */
/* ringSysop()    signal chat-mode request      */
/************************************************************************/

char getYesNo();

/************************************************************************/
/* editText() handles the end-of-message-entry menu.     */
/* return TRUE  to save message to disk,           */
/*        FALSE to abort message, and           */
/*        ERROR if user decides to continue        */
/************************************************************************/
int editText(buf, lim, ii)
char *buf;
int lim, *ii;
{
char iChar();
char c, inSer;


   do {
      ra.outFlag = OUTOK;
      inSer = FALSE;
      mPrintf("\n Edit> ");

      if(ra.haveCarrier || ra.onConsole) {
      switch (c= (char) toUpper(iChar())) {

   case 'A':
      mPrintf("bort\n ");
      if (getYesNo("OK")) return FALSE;
      break;

   case 'C':
      mPrintf("ontinue\n ");
      return ERROR;

   case 'F':
      mPrintf("ix Case");
      fakeFullCase(buf);
      doCR();
      mFormat(buf);
      break;

   case 'I':
      mPrintf("nsert");
      inSer = TRUE;
      replaceString(buf,lim,inSer, ii);
      break;

   case 'P':
      mPrintf("rint");
      doCR();
      if (ra.loggedIn && !(ra.roomBuf.rbflags & ANON)){
         mPrintf(   "  ");
         printDate();
         sprintf(format, "from %s", ra.msgBuf.mbauth);
         mPrintf(format);
         doCR();
         }
      mFormat(buf);
      break;

   case 'R':
      mPrintf("eplace");
      replaceString(buf, lim, inSer, ii);
      break;

   case 'S':
      mPrintf("ave\n ");
      doCR();
      if (ra.use_headline)
         getString("Message Headline ", ra.msgBuf.mbHeadline, 58);
   case 0:
      if ((ra.haveCarrier || ra.onConsole) && ( time(NULL) <= timeout) )
         return TRUE;
      else
         return FALSE;

   default:
      tutorial("edit.mnu");
      break;
   }
  }
  else
   return FALSE;

 }
while (ra.haveCarrier  ||  ra.onConsole);

return FALSE;

}

/************************************************************************/
/* findRoom() returns # of free room if possible, else ERROR   */
/************************************************************************/
int findRoom() {
int roomRover;

for (roomRover=0;  roomRover<MAXROOMS;  roomRover++) {
   if (!(ra.roomTab[roomRover].rtflags & INUSE)) return roomRover;
   }
return ERROR;
}

/************************************************************************/
/* getNumber() prompts for a number in (bottom, top) range. */
/************************************************************************/
int getNumber(prompt, bottom, top)
char   *prompt;
unsigned bottom;
unsigned top;
{
unsigned try, j;
char numstring[NAMESIZE];

pause_override = TRUE;
scrn = 0;

   do {
   ra.outFlag = OUTOK;
   getstring(prompt, numstring, NAMESIZE);
   try   = atoi(numstring);
   if (try < bottom)
      {  
/*      sprintf(format, "Must be greater than %d\n ", bottom - 1); */
      for (j = 1; j <= strlen(numstring); j++)
          mPrintf(" \b\b");
      }
   if (try > top  ) 
      {  
/*      sprintf(format, "Must be less than %d\n ", top + 1); */
      for (j = 1; j <= strlen(numstring); j++)
          mPrintf(" \b\b");
      }
   } 
while ((try < bottom ||  try > top) && (ra.haveCarrier  ||  ra.onConsole));

pause_override = FALSE;

return  try;
}

/************************************************************************/
/* getRoom()                     */
/************************************************************************/
getRoom(rm, buf)
int rm;
int *buf;
{
int      _read();
unsigned val;
unsigned long tmp;
tmp = (unsigned long) rm;
tmp *= SECSPERROOM;

/* load room #rm into memory starting at buf */
ra.thisRoom   = rm;
lseek(ra.roomfl,  tmp, SEEK_SET);
if ((val = _read(ra.roomfl, &ra.roomBuf, SECSPERROOM)) >= 10000)  {
   sprintf(format, "Failure on Goto Room %d\n\r ", val);
   doStatusLine(S_MSGS, format);
   }
return TRUE;
}

/************************************************************************/
/* getString() gets a string from the user.        */
/************************************************************************/
getString(prompt, buf, lim)
char *prompt;
char *buf;
int  lim;   /* max # chars to read */
{
char iChar();
char c;
int  i;

ra.outFlag = OUTOK;

pause_override = TRUE;
scrn = 0;

if(strLen(prompt) > 0) {
   sprintf(format, "\n Enter %s: ", prompt);
   mPrintf(format);
   }

i = 0;
while ( c = iChar(), c   != NEWLINE && 
        i   <  lim && (ra.haveCarrier || ra.onConsole)) {

   ra.outFlag = OUTOK;

   /* handle delete chars: */
   if (c == BACKSPACE) {
      oChar(' ');
      oChar(BACKSPACE);
      if (i > 0) i--;
      else  {
         oChar(' ');
         oChar(BELL);
         }
      }    
   else    if (c=ra.filter[c]) buf[i++] = c;  /* catch those nulls */

      if (i >= lim) {
         i--;
         oChar(BELL);
         oChar(BACKSPACE);
         oChar(' ');
         oChar(BACKSPACE);
         }

   /* kludge to return immediately on single '?': */
   if (*buf == '?')   {
      doCR();
      break;
      }
   }
buf[i]  = '\0';

pause_override=FALSE;

return TRUE;
}

/************************************************************************/
/* getText() reads a message from the user         */
/* Returns TRUE if user decides to save it, else FALSE      */
/************************************************************************/
char getText(prompt, buf, lim)
char *prompt;
char *buf;
int  lim;   /* max # chars to read */
{
char iChar(), visible();
char c, sysopAbort;
int  i, toReturn;

ra.outFlag = OUTOK;
if (!ra.expert)    tutorial("entry.blb");
ra.outFlag = OUTOK;
doCR();

if (ra.ansi_on) doANSI(ra.ANSI_header);

if (!(ra.roomBuf.rbflags & ANON)){
   mPrintf(   "   ");
   printDate();
   if (ra.loggedIn) 
      {  
      if (ra.ansi_on) doANSI(ra.ANSI_author);
      sprintf(format, "from %s", ra.msgBuf.mbauth);
      mPrintf(format);
      }
   doCR();
   }
lim--;
i  = 1;
buf[0] = ' ';
toReturn   = TRUE;
sysopAbort = FALSE;
if (ra.ansi_on) doANSI(ra.ANSI_text);
   do { 
      /* this code will handle the modem as well...  */
         /* who like to upload fast without handshaking  */
         /* fastIn() is now a piece of arcane history   */
         while (
            !(  (c=iChar()) == NEWLINE   &&   buf[i-1] == NEWLINE )
            && i < lim && (ra.haveCarrier || ra.onConsole)
/*            && ra.onConsole */
            ) {
            if (c != NEWLINE) c = ra.filter[c];
            else scrn = 0;
            if (c && c != BACKSPACE) buf[i++]   = c; /* catch nulls */
            if (c == BACKSPACE) {
               /* handle delete chars: */
               oChar(' ');
               oChar(BACKSPACE);
               if (i>0  && buf[i-1] != NEWLINE)   i--;
               else  oChar(BELL);
               }
            if (c)   
               timeout = time(NULL) + 
                         (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
            }

         buf[i] = 0x00;        /* null to terminate message */

      if (i == lim)  mPrintf("\007\n FULL");
     
   
   toReturn    =  sysopAbort  ?  FALSE  :  editText(buf, lim, &i);
   } 
while ((toReturn == ERROR)  &&  (ra.haveCarrier || ra.onConsole));
return  toReturn;
}

/************************************************************************/
/* getYesNo() prompts for a yes/no response        */
/************************************************************************/
char getYesNo(prompt)
char *prompt;
{
char iChar(), carrDet(), save_echo;
int  toReturn;

ra.outFlag = OUTOK;
toReturn = ERROR;

doCR();
sprintf(format, "%s? (Y/N): ", prompt);
mPrintf(format);

save_echo = ra.echo;

ra.echo = NEITHER; /* squash carriage returns */

for ( ; toReturn == ERROR   && (carrDet() || ra.onConsole); ) {

   switch ( (char) toUpper(iChar())) {
   case 'Y': 
      toReturn   = TRUE ;    
      oChar('Y');
      break;
   case 'N': 
      toReturn   = FALSE;    
      oChar('N');
      break;
   case 0:
 /*     toReturn   = FALSE;  */
      break;   
   default : /* oChar('\b'); oChar(' '); oChar('\b');*/  break;   
   }
 }
ra.echo = save_echo;             /* let carriage returns work again */

doCR();

if (toReturn == ERROR) toReturn=FALSE;

return   toReturn;
}

/************************************************************************/
/* givePrompt() prints the usual "CURRENTROOM>" prompt.     */
/************************************************************************/
givePrompt() {
int tOn;
char ss[12];

static int bauds[] = { 300, 1200, 2400, 4800, 9600, 14400};

doCR();

if(!ra.expert) {
   clr_screen = FALSE; /* tutorial() resets this flag to TRUE */
   if (ra.roomBuf.rbflags & CPMDIR && !ra.twit) tutorial("mnudir.blb");
   else tutorial("mnu.blb");
}

if (ra.loggedIn) 
   { 
   
/*   if(ra.baud > B9600) strcpy(ss, "HISPEED");
   else */
      if(ra.baud == -1) strcpy(ss, "CONSOLE");
      else 
         ltoa(ra.baud, ss, 10);

   doStatusLine(USER, ra.logBuf.lbname);
   doStatusLine(MSPEED, ss);
   if (!ra.onConsole && (tOn=timeOn()) > (ra.time_allowed + 1) ){
      sprintf(format, "%d minutes OVER LIMIT\n ", tOn-ra.time_allowed);
      mPrintf(format);
      if (tOn > (ra.time_allowed + 5)) tutorial("limit.blb");
      if (tOn > (ra.time_allowed + 15)) terminate(TRUE);
      }
   }


if(ra.whichIO != MODEM) doStatusLine(WHICH_IO,"console");

if(ra.ansi_on) {
  doANSI(ra.ANSI_bkgd);
  doANSI(ra.ANSI_prompt);
}  

if(!ra.loggedIn) mPrintf("\n Press 'L' to Login\n \n ");
if(ra.roomBuf.rbflags & MSG_READ_ONLY) mPrintf ("[Read Only]\n ");
sprintf(format, "%s <n,e,g,",ra.roomBuf.rbname);

mPrintf(format);

if (ra.roomBuf.rbflags & CPMDIR) { 
   if (!ra.twit) mPrintf("v,f");
   }

mPrintf(" ?");

if (!(ra.roomBuf.rbflags & PUBLIC)) {
   if( ra.roomBuf.rbflags & BY_INVITE_ONLY)
     mPrintf("  *> ");
   else
      mPrintf(" {  ");
}
else mPrintf(" > ");

if(ra.roomBuf.rbflags & CPMDIR && !ra.twit) mPrintf("\b\b] ");

if (ra.roomBuf.rbflags & ANON) mPrintf("\b- ");


/*if (!pause_override && ra.termMore)*/ scrn = 0;
pause_override = FALSE;

return TRUE;
}

/************************************************************************/
/* indexRooms() -- build RAM index to room.buf        */
/************************************************************************/
indexRooms() {
   int goodRoom, m, roomCount, slot;

   roomCount  = 0;
   for (slot=0;  slot<MAXROOMS;  slot++) {
      getRoom(slot, &ra.roomBuf);
     
      if (ra.roomBuf.rbflags & INUSE) {
         ra.roomBuf.rbflags ^=  INUSE;       /* clear "inUse" flag */

         for (m=0, goodRoom=FALSE; m<MSGSPERRM && !goodRoom; m++) {

            if (ra.roomBuf.msg[m].rbmsgNo >= ra.oldestLo) {
               goodRoom   = TRUE;
            }
         }

         if (goodRoom   ||   (ra.roomBuf.rbflags & PERMROOM)) {
            ra.roomBuf.rbflags |= INUSE;
         }

         if (ra.roomBuf.rbflags & INUSE) roomCount++;
         else {
            ra.roomBuf.rbflags = 0;
            putRoom(slot, ra.roomBuf);
            }
         }
      noteRoom();
      }
   return TRUE;
}

/************************************************************************/
/* interact() allows the sysop to interact directly with    */
/* whatever is on the modem.               dvm 9-82 */
/************************************************************************/
interact() {
   char c, lineEcho, lineFeeds, localEcho;
   char getMod(), /* getCh(),*/ mIready(), KBReady();
   int  xtop, xbottom, ytop, ybottom, split_win, i, ok;

clrscr();
split_win = ra.ti.screenheight/2 + 1;
window(1, 1, (int) ra.ti.screenwidth, 1);
textcolor(1);
textbackground(7);
gotoxy(1, 1);
for (i = 1; i <= (int) ra.ti.screenwidth; i++) cprintf("=");
gotoxy(7,1);
cprintf("%s", ra.logBuf.lbname);

window(1, split_win, (int) ra.ti.screenwidth, split_win);
textcolor(1);
textbackground(7);
gotoxy(1, 1);
for (i = 1; i <= (int) ra.ti.screenwidth; i++) cprintf("=");
gotoxy(7,1);
cprintf("Console");

xtop = ytop = xbottom = xtop = 1;  /* initialize window cursors */

window(1, split_win + 1, (int) ra.ti.screenwidth, (int) ra.ti.screenheight - 1);
textcolor(11);
textbackground(1);
clrscr();
window(1, 2, (int) ra.ti.screenwidth, split_win - 1);
textcolor(11);
textbackground(1);
clrscr();

lineEcho=localEcho=lineFeeds=TRUE;
sprintf(format, " Hi %s \n ", ra.logBuf.lbname);
mPrintf(format);


   /* incredibly ugly code.  Rethink sometime: */
   for(; ;) {
      c = 0;
    
      if (mIready() ) {
         window(1, 2, (int) ra.ti.screenwidth, split_win - 1);
         gotoxy(xtop, ytop);
         c  = ra.filter [getMod() & 0x7F];
         if (c != '\n') {
            if (lineEcho) oChar(c);
            else putch(c);
         }
         else{
            putch('\r');
            putch('\n');
            if (!lineFeeds) {
               if (lineEcho) outMod(13);
               }
            else{
               if (lineEcho) {
                  outMod(13);
                  outMod(10);
               }
           }
         }
      xtop = wherex();
      ytop = wherey();
      }

      if (KBReady()) { /* maybe reset timer */
         window(1, split_win + 1, 
                (int) ra.ti.screenwidth, (int) ra.ti.screenheight - 1);
         gotoxy(xbottom, ybottom);
         c = getCh();
         c  = ra.filter[c];
         if (c == SPECIAL) {
            getDaTime();
            ra.lhour = ra.hour;
            ra.lminute = ra.minute;
            break;
            }
         if (c != NEWLINE) {
            outMod(c);
            if (localEcho) putch(c);
            }     
         else 
/*            doCR();   */
           { putch('\r');
             putch('\n');
            if (!lineFeeds) {
               if (lineEcho) outMod(13);
               }
            else{
               if (lineEcho) {
                  outMod(13);
                  outMod(10);
               }                  
           }
          } 
         xbottom = wherex();
         ybottom = wherey();
         }
      fix_rollover();
      }
   if(ra.loggedIn) {
      mPrintf("bye\n ");
      ra.onConsole = ra.whichIO = MODEM;
      setUp(FALSE);
      }


      
   if(p_port){
     cprintf("\n\nTurn Hard Copy Off ? ");
     do{
       c = (char) toUpper(getCh());
     }
     while(c != 'Y' && c != 'N');
   
     if( c == 'Y') p_port = 0;
  }     

ok = gettext( 1, (int) ra.ti.screenheight,
     (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

window(1, 1, (int) ra.ti.screenwidth, (int) ra.ti.screenheight - 1);
textcolor(11);
textbackground(1);
gotoxy(1,1);
clrscr();
if(ok){
  puttext( 1, (int) ra.ti.screenheight,
         (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
  doStatusLine(WHICH_IO, " MODEM ");
  }
return TRUE;
}

/************************************************************************/
/* makeRoom() constructs a new room via dialogue with user. */
/************************************************************************/
makeRoom() {
   char getYesNo();
   char *nm[NAMESIZE];
   char *oldName[NAMESIZE];
   int  i;

   /* update lastMessage for current room: */
   ra.logBuf.lbgen[ra.thisRoom] = ra.roomBuf.rbgen << GENSHIFT;

   strcpy(oldName, ra.roomBuf.rbname);
   if ((ra.thisRoom=findRoom()) == /*ERROR*/ 255) {
      indexRooms();  /* try and reclaim an empty room */
      if ((ra.thisRoom=findRoom()) == /*ERROR*/ 255) {
         mPrintf("FULL\n ");
         /* may have reclaimed old room, so: */
         if (roomExists(oldName) == ERROR)  strcpy(oldName, "Lobby");
         getRoom(roomExists(oldName), &ra.roomBuf);
         return;
         }
      }

   getString("name", nm, NAMESIZE);
   normalizeString(nm);
   if (roomExists(nm) >= 0 || !strlen(nm) || *nm == '?') {
      mPrintf("INUSE\n ");
      /* may have reclaimed old room, so: */
      if (roomExists(oldName) == ERROR)   strcpy(oldName, "Lobby");
      getRoom(roomExists(oldName), &ra.roomBuf);
      return;
      }
   if (!ra.expert)   tutorial("newroom.blb");
   getRoom(ra.thisRoom);     /* bp's hidden fix */
   ra.roomBuf.rbflags = INUSE;
   if (getYesNo("Public"))    ra.roomBuf.rbflags |= PUBLIC;

   sprintf(format, "'%s', is %s",
      nm,
      ra.roomBuf.rbflags & PUBLIC  ?  "public"  :  "private"
      );
   mPrintf(format);
   if(!getYesNo("OK")) {
      /* may have reclaimed old room, so: */
      if (roomExists(oldName) == ERROR)   strcpy(oldName, "Lobby");
      getRoom(roomExists(oldName), &ra.roomBuf);
      return;
      }

   if ((ra.logBuf.lbnulls &  SYSOP) && ra.loggedIn) 
      if (getYesNo("Make Room Read Only") ) 
         ra.roomBuf.rbflags |= MSG_READ_ONLY;
         
   strcpy(ra.roomBuf.rbname, nm);
   for (i=0;  i<MSGSPERRM;  i++) {
      ra.roomBuf.msg[i].rbmsgNo   = 0;    /* mark all slots empty */
      ra.roomBuf.msg[i].rbmsgLoc  = FALSE /*ERROR*/;
      }
   ra.roomBuf.rbgen = (ra.roomBuf.rbgen +1) % MAXGEN;

   noteRoom();         /* index new room */
   putRoom(ra.thisRoom, ra.roomBuf);

   /* update ra.logBuf: */
   ra.logBuf.lbgen[ra.thisRoom] = ra.roomBuf.rbgen << GENSHIFT;
   if(!ra.onConsole){
      sPrintf(ra.msgBuf.mbtext, ">%s.",nm);
      logMsg();
      }
   tutorial("roominfo.blb");   
   return TRUE;
   }

/************************************************************************/
/* matchString() searches for match to given string.  Runs backward*/
/* through buffer so we get most recent error first.     */
/* Returns loc of match, else ERROR          */
/************************************************************************/
char *matchString(buf, pattern, bufEnd)
char *buf, *pattern, *bufEnd;
{
char *loc, *pc1, *pc2;
char foundIt;

for (loc=bufEnd, foundIt=FALSE;  !foundIt && --loc>=buf;) {
   for (pc1=pattern, pc2=loc,  foundIt=TRUE ;  *pc1 && foundIt;) {
      if (! (toLower(*pc1++) == toLower(*pc2++)))   foundIt=FALSE;
      }
   }

return   (foundIt  ?  loc  :  ERROR);
}

/************************************************************************/
/* normalizeString() deletes leading & trailing blanks etc. */
/************************************************************************/
normalizeString(s)
char *s;
{
char *pc, *s2;

pc = s;

/* find end of string   */
while (*pc)   {
   if (*pc < ' ')  *pc = ' ';   /* zap tabs etc... */
   pc++;
   }

/* no trailing spaces: */
while (pc>s  &&  isSpace(*(pc-1))) pc--;
*pc = '\0';

/* no leading spaces: */
while (*s == ' ') {
   for (pc=s;  *pc;  pc++)    *pc = *(pc+1);
   }

/* no double blanks */
for (;  *s;  s++)   {
   if (*s == ' '  &&   *(s+1) == ' ')   {
      for (pc=s; *pc;  pc++)    *pc = *(pc+1);
      }
   }
return TRUE;
}

/************************************************************************/
/* modemInit() is responsible for all modem-related initialization */
/* at system startup                */
/* Globals modified: ra.haveCarrier visibleMode    */
/*          ra.whichIO  ra.modStat     */
/*          ra.exitToCpm   ra.justLostCarrier   */
/* modified 82Dec10 to set FDC-1 SIO-B clock speed at    */
/* 300 ra.baud  -dvm                */
/************************************************************************/
modemInit() {
char c, carrDet();

ra.newCarrier    = FALSE;
/* don't like the blank in mail */
/*    visibleMode    = TRUE;     
*/    ra.exitToCpm      = FALSE;
ra.justLostCarrier  = FALSE;
ra.baud    = ra.hibaud;
ra.whichIO    = CONSOLE;


if (!ra.rcpm) {
  /* initPort();*/     /* hangUp also calls initPort() */
   hangUp();
   }
ra.haveCarrier = ra.modStat = /**interpret(pCarrDetect)*/carrDet();
return TRUE;
}

/************************************************************************/
/* noteRoom() -- enter room into RAM index array.        */
/************************************************************************/
noteRoom()
{
int i;
unsigned long int last;

last = 0;
for (i=0;  i<MSGSPERRM;  i++)  {
   if (ra.roomBuf.msg[i].rbmsgNo > last) {
      last = ra.roomBuf.msg[i].rbmsgNo;
      }
   }
ra.roomTab[ra.thisRoom].rtlastMessage = last        ;
strcpy(ra.roomTab[ra.thisRoom].rtname, ra.roomBuf.rbname) ;
ra.roomTab[ra.thisRoom].rtgen       = ra.roomBuf.rbgen  ;
ra.roomTab[ra.thisRoom].rtflags     = ra.roomBuf.rbflags;
return TRUE;
}

/************************************************************************/
/* putRoom() stores room in buf into slot rm in room.buf    */
/************************************************************************/
putRoom(rm, buf)
int rm;
int *buf;
{
int      _write();
unsigned val;
unsigned long tmp;

tmp = (unsigned long) rm;
tmp *= SECSPERROOM;
lseek(ra.roomfl, tmp, SEEK_SET);
if ((val = _write(ra.roomfl, &ra.roomBuf, SECSPERROOM)) != SECSPERROOM) {
   sprintf(format, "Error Putting Room%d", val);
   doStatusLine(S_MSGS, format);
   }
return TRUE;
}

/************************************************************************/
/* renameRoom() is sysop special fn          */
/* Returns: TRUE on success else FALSE       */
/************************************************************************/
renameRoom() {
char getYesNo();
char nm[60];
char c, goodOne, wasDirectory, ch, carrDet(), iChar();
int  r;
struct logBuffer lbuf;
unsigned char BIO;

if (ra.thisRoom <= LOGROOM   ) {
   mPrintf("Can't Edit A System Room\n ");
   return FALSE;
   }

if (ra.onConsole) strcpy(ra.msgBuf.mbtext, "CONSOLE: ");
else ra.msgBuf.mbtext[0] = '\0';
strcat(ra.msgBuf.mbtext, ra.roomBuf.rbname);
strcat(ra.msgBuf.mbtext, "\n ");
if (ra.loggedIn) strcat(ra.msgBuf.mbtext, ra.logBuf.lbname);
strcat(ra.msgBuf.mbtext, "\n ");

/* if (!getYesNo("OK"))   return FALSE; */

doRenameMenu();

ch = ' ';
while (ch != 'X' && (carrDet() || ra.onConsole)) {
   mPrintf("Selection : ");
   
   ch = (char) toupper(iChar() );
   doCR();   
   switch(ch) {
    case 'A':
    if (getYesNo("Rename Room"))   {
      getString("New", nm, NAMESIZE);
      normalizeString(nm);
      r = roomExists(nm);
      if (r>=0  &&  r!=ra.thisRoom) {
         mPrintf("Already Exists\n ");
      }
      else if (!strlen(nm) ) {
           mPrintf("\n Name Not Changed\n ");       
           }
           else {
              sprintf(format, "%s to %s\n ", ra.roomBuf.rbname, nm);
              strcpy(ra.roomBuf.rbname,      nm);   /* also in room itself  */
              strcat (ra.msgBuf.mbtext, format);
          }
   }
   break;

   case 'B' :
      {
      if (ra.roomBuf.rbflags & BY_INVITE_ONLY) {
         mPrintf("\n Room Is By Invitation Only, Must Turn off BIO Attribute First\n");
         break;
         }
         
      ra.roomBuf.rbflags ^= PUBLIC;
      if (!(ra.roomBuf.rbflags & PUBLIC)) {
         mPrintf("\n This will Cause All Users of this Room To See All Messages As New\n ");
         if (!getYesNo("OK ")) {
            ra.roomBuf.rbflags |= PUBLIC;
            break;
         }
         ra.roomBuf.rbgen   = (ra.roomBuf.rbgen +1) % MAXGEN; 
      }   
   
      sprintf(format, " Room Is Now %s\n ",
         (ra.roomBuf.rbflags & PUBLIC) ? "PUBLIC" : "PRIVATE");
      mPrintf(format);
      if (!(ra.roomBuf.rbflags & PUBLIC)) ra.roomBuf.rbflags &= ~ANON;
      strcat(ra.msgBuf.mbtext, format);
      break;
      }
   case 'D' :         
      if (ra.roomBuf.rbflags & PUBLIC ){
            ra.roomBuf.rbflags ^= MSG_READ_ONLY;
            sprintf(format," Room Is Now %s\n ",
               (ra.roomBuf.rbflags & MSG_READ_ONLY) ? "READ ONLY" : "R/W");
            mPrintf(format);
            strcat(ra.msgBuf.mbtext, format);
               
          }
      else mPrintf(" Only Public Rooms Can Be Read Only\n ");         
      break;

   case 'C' :
      if (!(ra.onConsole || ra.grand_poobah)) break;
      if (getYesNo("Invitation Only")) {
         ra.roomBuf.rbflags |= BY_INVITE_ONLY;
         ra.roomBuf.rbflags &= ~PUBLIC;
      }
      else ra.roomBuf.rbflags &= ~BY_INVITE_ONLY;   
      if ((ra.roomBuf.rbflags & BY_INVITE_ONLY)) {
         mPrintf("\n This will Cause All Users of this Room To See All Messages As New\n ");
         if (!getYesNo("OK")) break;
         
         ra.roomBuf.rbgen   = (ra.roomBuf.rbgen +1) % MAXGEN; 
         }
      sprintf(format, "Room is now %s\n ",
         (ra.roomBuf.rbflags & BY_INVITE_ONLY) ? "BIO" : "Not BIO");
      mPrintf(format);
      strcat(ra.msgBuf.mbtext, format);   
      break;
      
   case 'E' :
   case 'F' :
      mPrintf("\n Not Available\n ");
      break;
      
   case 'G' :
      if (!(ra.onConsole || ra.grand_poobah)) {
         mPrintf("\n Can't Do This Function Remotely\n ");
         break;
         }
      if (getYesNo("Make Directory Room ")) {
         ra.roomBuf.rbflags    |= CPMDIR;
         ra.roomBuf.rbflags    |= PERMROOM;
         ra.roomBuf.rbflags    &= ~NO_UPLOADS;
         }
      else {
        ra.roomBuf.rbflags &= ~CPMDIR;
        break;
      }  
      sprintf(format, "Room is now %s\n ", (ra.roomBuf.rbflags & CPMDIR)
             ? "Directory" : "Non-directory");
      mPrintf(format);
      strcat(ra.msgBuf.mbtext, format);       
   case 'H' :       
      if (!(ra.onConsole || ra.grand_poobah)) {
         mPrintf("\n Can't Do This Function Remotely\n ");
         break;
         }
      if (!ra.roomBuf.rbflags & CPMDIR) break;
      sprintf(format, " Directory is now: %c:%s\n\r ", 
         ('A'+ra.roomBuf.rbdisk), ra.roomBuf.rbuser);
      mPrintf(format);
      if (!(&ra.roomBuf.rbuser[0]))
         if(getYesNo(" Do You Wish To Change Directory ")) break;
      for (goodOne=FALSE;  !goodOne;   )   {
         getString("Disk letter", nm, NAMESIZE);
         c     = (char) toUpper(nm[0]);
         if (c>='A' && c<='P') {
            ra.roomBuf.rbdisk     = c - 'A';
            goodOne      = TRUE;
            }     
         else mPrintf("?");
         }

         getString("Directory (From Root)", nm, 60);
      normalizeString(nm);
      strcpy(ra.roomBuf.rbuser, nm);
      nm[0] = c;
      nm[1] = ':';
      nm[2] = '\0';
      strcat(nm, ra.roomBuf.rbuser);
      if(mkdir(nm) == -1){ 
         mPrintf("Creation of directory failed, either already present, or disk error");
         doCR();
    
      if(getYesNo("Do you wish to use this directory anyway")){
         ra.roomBuf.rbflags |= CPMDIR;
         sprintf(format," %c:%s\n\n", 
             ('A'+ra.roomBuf.rbdisk), ra.roomBuf.rbuser);
         mPrintf(format);    
       }
    else ra.roomBuf.rbflags &= ~CPMDIR;   
         }
      else {
         sprintf(format," %c:%s\n\n", 
           ('A'+ra.roomBuf.rbdisk), ra.roomBuf.rbuser);
         mPrintf(format);
         }  
         strcat(ra.msgBuf.mbtext, format);
      break;   
      

     case 'I' :
      if (!(ra.onConsole || ra.grand_poobah)) {
         mPrintf("\n Can't Do This Function Remotely\n ");
         break;
         }
      if (ra.roomBuf.rbflags & CPMDIR) {
         if (getYesNo("No Uploads Allowed") ) 
            ra.roomBuf.rbflags |= NO_UPLOADS;
         else ra.roomBuf.rbflags &= ~NO_UPLOADS;
         }
      sprintf(format, "Directory Allows %s\n ", (ra.roomBuf.rbflags & NO_UPLOADS) 
          ? "No Uploads" : "Uploads");
      mPrintf(format);
      strcat(ra.msgBuf.mbtext, format);
      break;                            
     case 'J' :
       if ( !(ra.roomBuf.rbflags & CPMDIR)) {
          if( getYesNo("Make Room Permanent"))
             ra.roomBuf.rbflags    |= PERMROOM;
          else ra.roomBuf.rbflags &= ~PERMROOM;   
        }
        else mPrintf("\n Directory Rooms Are Always Permanent\n ");
        sprintf(format, "Room is %s Permanent\n ",
           ra.roomBuf.rbflags & PERMROOM ? "  " : "Not");
        mPrintf (format);
        strcat(ra.msgBuf.mbtext, format);
        break;

     case 'K' :
        if (ra.roomBuf.rbflags & PUBLIC){
           if (getYesNo("Anonymous Messages")) ra.roomBuf.rbflags |= ANON;
           else ra.roomBuf.rbflags &= ~ANON;
           }
        sprintf(format, "Room is %s \n ",
           ra.roomBuf.rbflags & ANON ? "Anonymous" : "Not Anonymous");
        mPrintf (format);
        strcat(ra.msgBuf.mbtext, format);   
        break;   
     
     case 'X' : 
        logMsg();
        break;
     
     default: doRenameMenu(); break;
     }
  }

if ( !(ra.roomBuf.rbflags & CPMDIR)) {
   ra.roomBuf.rbflags |= NO_UPLOADS;
   ra.roomBuf.rbuser[0] = '\0';
   ra.roomBuf.rbdisk = ra.homeDisk;
   }
        
noteRoom();

/* ra.roomBuf.rbgen   = (ra.roomBuf.rbgen +1) % MAXGEN; */

if(ra.roomBuf.rbflags & BY_INVITE_ONLY) {
 ra.logBuf.BIO_waste[ra.thisRoom/8] |= bits[ra.thisRoom%8];
 for ( r = 0; r < MAXLOGTAB; r++) {
     getLog(&lbuf, r);
     if (lbuf.lbnulls & SYSOP)
        lbuf.BIO_waste[ra.thisRoom/8] |= bits[ra.thisRoom%8];
     else
        lbuf.BIO_waste[ra.thisRoom/8] &= ~(bits[ra.thisRoom%8]);
     putLog(&lbuf, r);
 }
}

putRoom(ra.thisRoom, ra.roomBuf);
 
return TRUE;
}

/*************************************************

doRenameMenu() now displays room edit options

*************************************************/

doRenameMenu()

{doCR();
 sprintf(format," A) Room Name : %s\n ", &ra.roomBuf.rbname);
 mPrintf(format);
 sprintf(format,"B) Public : %s\n ", 
        ( ra.roomBuf.rbflags & PUBLIC) ? "YES" : "NO");
 mPrintf(format);
 sprintf(format,"C) Invitation Only : %s\n ",
        (ra.roomBuf.rbflags &  BY_INVITE_ONLY) ? "YES" : "NO");
 mPrintf(format);
 sprintf(format,"D) Read Only : %s\n ",
        ( ra.roomBuf.rbflags & MSG_READ_ONLY) ? "YES" : "NO");
 mPrintf(format);       
 mPrintf("E) Network : NONE\n ");
 mPrintf("F) FIDONet Message Directory : NONE\n ");
 sprintf(format, "G) File Room : %s\n ",
        (ra.roomBuf.rbflags & CPMDIR) ? "YES" : "NO");
 mPrintf(format);
 sprintf(format, "H) File Directory : %c:%s\n ", 
        ('A'+ra.roomBuf.rbdisk), ra.roomBuf.rbuser);
 mPrintf(format);
 sprintf(format, "I) Uploads Allowed : %s\n ",
        ( ra.roomBuf.rbflags & NO_UPLOADS) ? "NO" : "YES");
 mPrintf(format);
 sprintf(format, "J) Permanent Room : %s\n ",
        ( ra.roomBuf.rbflags & PERMROOM) ? "YES" : "NO");
 mPrintf(format);       
 sprintf(format, "K) Anonymous Messages : %s\n ",
        ( ra.roomBuf.rbflags & ANON) ? "YES" : "NO");
 mPrintf(format);
 mPrintf("X) eXit\n \n ");
 }
          
/************************************************************************/
/* replaceString() corrects typos in message entry       */
/************************************************************************/
replaceString(buf, lim, inSer, i)
char *buf;
int  lim, *i;
char inSer;
{
char oldString[2*SECTSIZE];
char newString[2*SECTSIZE];
char *loc, *textEnd;
char *pc;
int  incr;

for (textEnd=buf;  *textEnd;  textEnd++);   /* find terminal null   */
sprintf(format, " %s: ",(inSer) ?
       "Insert Paragraph Where " : " What ");
mPrintf(format);
getString("",      oldString, (2*SECTSIZE));
if (!strlen(oldString) ) return;
if ((loc=matchString(buf, oldString, textEnd)) == ERROR) {
   mPrintf("\n String Not Found\n ");
   return;
   }
if (inSer) {  /* insert paragraph */
   for (pc=textEnd; pc>=loc; pc--) { /* make space for ' \n' */
      *(pc+2) = *pc;
      }
   *loc++ = '\n';
   *loc++ = ' ';
   *i += 2;
   return;
   }

mPrintf("\n Replace With: ");
getString("", newString, (2*SECTSIZE));
if ( ((long) strLen(newString) - (long) strLen(oldString))
      >=  ( (long) &buf[lim] - (long) textEnd) ) {
   mPrintf("\n Not Enough Room in Buffer\n ");
   return;
   }
/* delete old string: */
for (pc=loc, incr=strLen(oldString);  *pc=*(pc+incr);  pc++);
textEnd -= incr;

/* make room for new string: */
for (pc=textEnd, incr=strLen(newString);  pc>=loc;   pc--) {
   *(pc+incr) = *pc;
   }

/* insert new string: */
for (pc=newString;  *pc;  *loc++ = *pc++);
*i += (strlen(newString) - strlen(oldString));
return TRUE;
}

/************************************************************************/
/* ringSysop() signals a chat mode request.  Exits on input from  */
/*  modem or keyboard.                    */
/************************************************************************/
ringSysop() {
char BBSCharReady(), getMod(), KBReady(), a, b;
char /**interpret()*/carrDet(), i;

if(!ra.loggedIn)
  {
   mPrintf("\n Must Log-in To Chat \n ");
   return;
  }
 
i = 0;
   
while ((!KBReady() ) && (!(BBSCharReady()) ) && carrDet() )
  {  i = ++i % 3;
     /* play shave-and-a-haircut/two bits... as best we can: */
     oChar(BELL);
     pause(3);
     if ( i == 0) pause(3);
   }
if  (KBReady())   {
   getCh();

   interact();

   return;    
   }

if(BBSCharReady()){
   getMod();
   tutorial("nochat.blb");
   }
return TRUE;
}


