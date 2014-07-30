/************************************************************************/
/* Borland C++ 2.0 8/92                    */
/* bds1.46  9/89        msg.c          */
/*                         */
/* Message handling for Citadel bulletin board system    */
/************************************************************************/

/************************************************************************/
/*          history           */
/*                         */
/* 92Aug05 JCL Ported to MSDOS                        */
/* 87Sep19 bp   added one old msg with new                              */
/* 86DEC30 VC,BP  Fix tabs, fix fakeFull.          */
/* 86Apr23 BP,KK,MM  networks, etc              */
/* 83Mar03 CrT & SB   Various bug fixes...            */
/* 83Feb27 CrT Save private mail for sender as well as recipient. */
/* 83Feb23  Various.  transmitFile() won't drop first char on WC... */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov05 CrT Stream retrieval.  Handles messages longer than MAXTEXT.*/
/* 82Nov04 CrT Revised disk format implemented.       */
/* 82Nov03 CrT Individual history begun.  General cleanup.     */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*          contents          */
/*                         */
/* aideMessage()     saves auto message in Aide>      */
/* dGetWord()     reads a word off disk         */
/* dPrintf()      printf() that writes to disk     */
/* fakeFullCase()    converts uppercase message to mixed case*/
/* findPerson()      load log record for named person */
/* flushMsgBuf()     wraps up message-to-disk store      */
/* getMessage()      load message into RAM         */
/* getMsgChar()      returns successive chars off disk   */
/* getMsgStr()    reads a string out of message.buf   */
/* getWord()      gets one word from message buffer   */
/* mAbort()    checks for user abort of typeout */
/* makeMessage()     menu-level message-entry routine */
/* mFormat()      formats a string to modem and console  */
/* mPrintf()      writes a line to modem & console */
/* mWCprintf()    special mPrintf for WC transfers */
/* noteLogMessage()  enter message into log record    */
/* noteMessage()     enter message into current room  */
/* note2Message()    noteMessage() local        */
/* printMessage()    prints a message on modem & console */
/* pullIt()    sysop special message-removal routine  */
/* putMessage()      write message to disk         */
/* putMsgChar()      writes successive message chars to disk */
/* putWord()      writes one word to modem & console  */
/* showMessages()    menu-level show-roomful-of-messages fn */
/* startAt()      setup to read a message off disk */
/* unGetMsgChar()    return a char to getMsgChar()    */
/************************************************************************/

char putMessage();

/************************************************************************/
/* aideMessage() saves auto message in Aide>       */
/************************************************************************/
aideMessage(noteDeletedMessage)
char noteDeletedMessage;
{

int ourRoom;

/* message is already set up in ra.msgBuf.mbtext */
putRoom(ourRoom=ra.thisRoom, &ra.roomBuf);
getRoom(AIDEROOM, &ra.roomBuf);
ra.msgBuf.mbauth[0] = '\0';
ra.msgBuf.mbto[0] = '\0';
ra.msgBuf.mbHeadline[0] = '\0';
if (putMessage( /* uploading== */ FALSE))   noteMessage(0, ERROR);

if (noteDeletedMessage)   {
   note2Message(ra.pulledMId, ra.pulledMLoc);
   }

putRoom(AIDEROOM, &ra.roomBuf);
noteRoom();
getRoom(ourRoom, &ra.roomBuf);
return TRUE;
}

/************************************************************************/
/* dGetWord() fetches one word from current message, off disk  */
/* returns TRUE if more words follow, else FALSE         */
/************************************************************************/
char dGetWord(dest, lim)
char *dest;
int  lim;
{
char getMsgChar();
char c;

--lim;  /* play it safe */

/* pick up any leading blanks: */
for (c = getMsgChar();(c==TAB||c==' ') && c && lim;   c = getMsgChar()) {
   if (lim) { 
      *dest++ = c;   
      lim--; 
      }
   }

/* step through word: */
for (       ;c!=TAB && c!= ' ' && c && lim;   c = getMsgChar()) {
   if (lim) { 
      *dest++ = c;
      lim--; 
      }
   }

/* trailing blanks: */
for (       ;(c==TAB||c==' ') && c && lim;   c = getMsgChar()) {
   if (lim) { 
      *dest++ = c;   
      lim--; 
      }
   }

if (c)  unGetMsgChar(c);  /* took one too many */

*dest = '\0';    /* tie off string */

return  c;
}

/*----------------------------------------------------------------------*/
/*  dPrintf() write from format+args to disk       */
/*----------------------------------------------------------------------*/
dPrintf(dformat)       /* plus an unknown #arguments for format    */
char *dformat;
#define MAXWORD 256
{
char *s;


s = dformat;
while (*s) putMsgChar(*s++);
return TRUE;
}

/************************************************************************/
/* fakeFullCase() converts a message in uppercase-only to a */
/* reasonable mix.  It can't possibly make matters worse... */
/* Algorithm: First alphabetic after a period is uppercase, all   */
/* others are lowercase, excepting pronoun "I" is a special case. */
/* We assume an imaginary period preceding the text.     */
/*      Now catch 1st i and i after spaces and leave proper nouns .bp.  */
/************************************************************************/
fakeFullCase(text)
char *text;
{
char *c;
char lastWasPeriod;
char state;

for(lastWasPeriod=TRUE, c=text;   *c;  c++) {
   
   if ( *c != '.'  &&  *c != '?' && *c != '!' ) {
      if (isAlpha(*c)) {

        if (!islower(*(c+1)))   *c      = (char) toLower(*c);

        if (lastWasPeriod)   *c = (char) toUpper(*c);

        lastWasPeriod  = FALSE;
      }
   }    
   else {
      lastWasPeriod = TRUE ;
   }
}

   /* little state machine to search for ' i ': */
#define NUTHIN    0
#define FIRSTBLANK   1
#define BLANKI    2
   for (state=FIRSTBLANK, c=text;  *c;  c++) {
      switch (state) {
      
      case NUTHIN:
        if (isSpace(*c)) state = FIRSTBLANK;
        else    state = NUTHIN    ;
        break;
      
      case FIRSTBLANK:
        if (*c == 'i')   {  
           state = BLANKI    ;
           break;
        }
        
        if (isSpace(*c)) state = FIRSTBLANK;
        else    state = NUTHIN    ;
       
        break;

      case BLANKI:
         if (isSpace(*c)) state = FIRSTBLANK;
         else    state = NUTHIN    ;

         if (!isAlpha(*c))   *(c-1)   = 'I';
         break;
      }
   }
return TRUE;
}

/************************************************************************/
/* findPerson() loads log record for named person.       */
/* RETURNS: ERROR if not found, else log record #        */
/************************************************************************/
int findPerson(name, lBuf)
char         *name;
struct logBuffer    *lBuf;
{
int  h, i, foundIt, logNo;

h = hash(name);
for (foundIt=i=0;  i<MAXLOGTAB && !foundIt;  i++) {

   if (ra.logTab[i].ltnmhash == h) {
      getLog(lBuf, logNo = ra.logTab[i].ltlogSlot);

      if (strCmpU(name, lBuf->lbname) == SAMESTRING) {
         foundIt = TRUE;
      }
   }
}

if (!foundIt)    return ERROR;
else      return logNo;
}

/************************************************************************/
/* flushMsgBuf() wraps up writing a message to disk      */
/************************************************************************/
flushMsgBuf() {
   unsigned long tmp;
   tmp = (unsigned long) ra.thisSector;
   tmp *= SECTSIZE;
   lseek(ra.msgfl, tmp, SEEK_SET);
   if (_write(ra.msgfl, ra.sectBuf, SECTSIZE) != SECTSIZE) {
      cprintf("Msg Write Error");
      }
   return TRUE;
   }

/************************************************************************/
/* getMessage() reads a message off disk into RAM.       */
/* a previous call to setUp has specified the message.      */
/************************************************************************/
getMessage() {
char c, getMsgChar();

/* clear ra.msgBuf out */
ra.msgBuf.mbauth[ 0]   = '\0';
ra.msgBuf.mbdate[ 0]   = '\0';
ra.msgBuf.mbHeadline[ 0] = '\0';
/*    ra.msgBuf.mborig[ 0] = '\0';
    ra.msgBuf.mboname[0]   = '\0';
    ra.msgBuf.mbsrcId[0]   = '\0';
*/

ra.msgBuf.mbtext[ 0] = '\0';
ra.msgBuf.mbto[   0]   = '\0';

timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   

do 
  c = getMsgChar();
while ((unsigned) c != 0xFF);  /* find start of msg */

ra.msgBuf.mbheadChar   = ra.oldChar;     /* record location   */
ra.msgBuf.mbheadSector = ra.oldSector;

getMsgStr(ra.msgBuf.mbId, NAMESIZE);

   do   {
   c = getMsgChar();

   switch (c) {

   case 'A':   
      getMsgStr(ra.msgBuf.mbauth,  NAMESIZE);
      break;

   case 'D':   
      getMsgStr(ra.msgBuf.mbdate,  NAMESIZE);
      break;

   case 'M':   /* just exit -- we'll read off disk */
      break;

/* case 'N':   getMsgStr(ra.msgBuf.mboname, NAMESIZE);   break;
   case 'O':   getMsgStr(ra.msgBuf.mborig,  NAMESIZE);   break;
   case 'S':   getMsgStr(ra.msgBuf.mbsrcId, NAMESIZE);   break;
*/ 

   case 'T':   
      getMsgStr(ra.msgBuf.mbto,    NAMESIZE);   
      break;
   
   case 'H':   if (ra.use_headline) getMsgStr(ra.msgBuf.mbHeadline, 60);  break;
   
   default:
      getMsgStr(ra.msgBuf.mbtext, MAXTEXT); /* discard unknown field  */
      ra.msgBuf.mbtext[0] = '\0';
      break;
   }
 }
while (c != 'M'  &&  isAlpha(c));
          
return TRUE;
}

/************************************************************************/
/* getMsgChar() returns sequential chars from message on disk  */
/************************************************************************/
char getMsgChar() {
char visible();
char toReturn;
int  mark, val;
unsigned long tmp;

if (ra.GMCCache) {  /* someone did an unGetMsgChar() --return it */
   toReturn= ra.GMCCache;
   ra.GMCCache= '\0';
   return toReturn;
   }

ra.oldChar = ra.thisChar;
ra.oldSector  = ra.thisSector;

toReturn   = ra.sectBuf[ra.thisChar];

#ifdef XYZZY
if (ra.debug) cprintf("%c",visible(toReturn));
#endif

ra.thisChar   = ++ra.thisChar % SECTSIZE;

if (ra.thisChar == 0) {
   /* time to read next sector in: */
   ra.thisSector  = (++ra.thisSector) % ra.maxMSector;
   tmp = (unsigned long) ra.thisSector;
   tmp *= SECTSIZE;
   lseek(ra.msgfl, tmp, SEEK_SET);
   if (_read(ra.msgfl, ra.sectBuf, SECTSIZE) >= 5000) {
      cprintf("Read Msg. Error");
      }
   }

return(toReturn);
}

/************************************************************************/
/* getMsgStr() reads a string from message.buf        */
/************************************************************************/
getMsgStr(dest, lim)
char *dest;
int  lim;
{
char c;

while (c = getMsgChar()) {      /* read the complete string   */
   if (lim) {        /* if we have room then    */
      lim--;
      *dest++ = c;     /* copy char to buffer     */
      }
   }
*dest = '\0';       /* tie string off with null   */
return TRUE;
}

/************************************************************************/
/* getWord() fetches one word from current message       */
/************************************************************************/
int getWord(dest, source, offset, lim)
char *dest, *source;
int  lim, offset;
{
int i, j;

/* skip leading blanks if any */
for (i=0;  source[offset+i]==' ' && i<lim;  i++);

/* step over word */
for (; source[offset+i] != ' ' && i < lim && source[offset+i] != 0; i++);

/* pick up any trailing blanks */
for (;  source[offset+i]==' ' && i<lim;  i++);

/* copy word over */
for (j=0;  j<i;  j++)  dest[j] = source[offset+j];
dest[j] = 0;  /* null to tie off string */

return(offset+i);
}

/************************************************************************/
/* mAbort() returns TRUE if the user has aborted typeout    */
/* Globals modified: ra.outFlag           */
/************************************************************************/
char mAbort() {
char BBSCharReady(), iChar();
char c, toReturn;

/* Check for abort/pause/drop from user */
if((!carrDet())&&(ra.whichIO==MODEM))
   {
   ra.outFlag=OUTSKIP;
   return TRUE;
   }

if (!BBSCharReady()) {
   toReturn = FALSE;
   } 
else {
   ra.echo  = NEITHER;
   c = (char) toUpper(iChar());

   switch (c) {

   case XOFF:
   case 'P':                /*   pause:         */
      scrn = 0;
      c = iChar();            /* wait to resume */
      if ( (char) toLower(c) == 'd'  ) ra.pullMessage = TRUE;
      toReturn     = FALSE;
      break;

   case 'J':                /* jump paragraph:*/
      ra.outFlag = OUTPARAGRAPH;
      toReturn   = FALSE;
      break;

   case 'N':                /* next:         */
      if(no_stop) {
        toReturn = FALSE;
      }  
      else {
        ra.outFlag = OUTNEXT;
        toReturn   = TRUE;      
      }  
      break;

   case 'S':                /* skip:         */
      ra.outFlag = OUTSKIP;
      toReturn   = TRUE;
      break;

   default:
      toReturn   = FALSE;
      break;
   }
   ra.echo  = BOTH;
 }
return toReturn;
}

/************************************************************************/
/* makeMessage is menu-level routine to enter a message     */
/* Return: TRUE if message saved else FALSE        */
/************************************************************************/
makeMessage(uploading)
char uploading;   /* TRUE if message is coming via WC protocol */
{
char    putMessage();
char    *pc, toReturn;
char    toName[NAMESIZE], ucname[NAMESIZE];
struct  logBuffer   lBuf;
int     msgchrcnt, locasecnt, logNo;

sysopflag = toReturn = FALSE;
ra.msgBuf.mbtext[0] = 0;
logNo   = ERROR;/* not needed, but it's nice to initialize... */

if (ra.thisRoom != MAILROOM)  ra.msgBuf.mbto[0] = FALSE;
else {
   {
      if (ra.reply) {
         strcpy(ra.msgBuf.mbto, ra.msgBuf.mbauth);
         logNo = findPerson(ra.msgBuf.mbto, &lBuf);
         sprintf(format, "\n To: %s\n ",ra.msgBuf.mbto);
         mPrintf(format);
      }        /*see showMes*/
      else  {
      if ((!ra.loggedIn) || ra.twit ) {
         strCpy(ra.msgBuf.mbto, "Sysop");
         doCR();
         mPrintf("To: Sysop");
         doCR();
      }
      else   getString("To", ra.msgBuf.mbto, NAMESIZE);

         if(ra.justLostCarrier) return FALSE;
         normalizeString(ra.msgBuf.mbto);
         logNo   = findPerson(ra.msgBuf.mbto, &lBuf);

         strcpy(ucname, ra.msgBuf.mbto);    
         sToUpper(ucname);
         
         if ((logNo==ERROR  &&  hash(ucname)!=hash("SYSOP"))  ||
              strlen(ra.msgBuf.mbto) == 0) {
     
            sprintf(format, "No '%s'", ra.msgBuf.mbto);
            mPrintf(format);
            return FALSE;
         }
      }
   }
 }

/* send sysop mail to real sysops */
if(strlen(ra.msgBuf.mbto)) {
  normalizeString(ra.sysopname);
  if(hash(ucname) == hash("SYSOP")) {
    if((logNo = findPerson(ra.sysopname, &lBuf)) != ERROR){
      sysopflag = TRUE;
      strcpy(ra.msgBuf.mbto, ra.sysopname);
    }
  }
}
      
/*    if !uploading  */
strcpy(ra.msgBuf.mbauth, ra.logBuf.lbname);    /* record author*/

if (uploading || getText("message", ra.msgBuf.mbtext, MAXTEXT)) {

   if (!uploading)   {
  
      for (pc=ra.msgBuf.mbtext, locasecnt=0, msgchrcnt=0; *pc; pc++){
         msgchrcnt++;
        
         if ( (char) toUpper(*pc) != *pc) locasecnt++;
      }
      
      /* if < 25% msg is lower & msg > 40 char then fake it */
      if ((locasecnt<(msgchrcnt>>2)) && (msgchrcnt>40))

         fakeFullCase(ra.msgBuf.mbtext, MAXTEXT);
   }

   if (toReturn=putMessage(uploading))   noteMessage(&lBuf, logNo);
 }
return toReturn;
}

/************************************************************************/
/* mFormat() formats a string to modem and console       */
/************************************************************************/
mFormat(string)
char *string;
#define MAXWORD 256  /* maximum length of a word */
{
char wordBuf[MAXWORD];
char mAbort();
int  i;

for (i=0;  string[i] && (!ra.outFlag || ra.outFlag==OUTPARAGRAPH);  ) {
   i = getWord(wordBuf, string, i, MAXWORD);
   putWord(wordBuf);

   if (mAbort()) return;
   }
return TRUE;
}

/************************************************************************/
/*     mPrintf() formats format+args to modem and console   */
/************************************************************************/
#define MAXWORD 256  /* maximum length of a word */

mPrintf(mformat)
char mformat[MAXWORD];
{
char mAbort();
char wordBuf[MAXWORD];
int  i;

for (i=0;  mformat[i]  && (!ra.outFlag  || ra.outFlag==OUTPARAGRAPH);  ) {
   i = getWord(wordBuf, mformat , i, MAXWORD);
   putWord(wordBuf);

   if (mAbort()) return;
   }
return TRUE;
}

/************************************************************************/
/*  mWCprintf() formats format+args to sendWCChar()      */
/************************************************************************/
mWCprintf(char *buffer, ...) /* plus an unknown #arguments for format */
/* char *format; */
{/*
char *s;
char string[MAXWORD];


string[0] = ' ';
sprintf(string, format);

s = string;
do if (*s) sendWCChar(*s);
while (*s++);
return TRUE;
*/

va_list argptr;
int cnt;
va_start(argptr, buffer);
cnt = vsprintf(format, buffer, argptr);
va_end(argptr);
if(_write(upld, format, strlen(format)) != strlen(format)) return(FALSE);
else return (TRUE);
}

/************************************************************************/
/* noteLogMessage() slots message into log record        */
/************************************************************************/
noteLogMessage(lBuf, logNo)
struct logBuffer   *lBuf;
int         logNo;
{
int i;

/* store into recipient's log record: */
/* slide message pointers down to make room for this one: */
for (i=0;  i<MAILSLOTS-1;  i++) {
   (*lBuf).lbslot[i]   = (*lBuf).lbslot[i+1];
   (*lBuf).lbId[  i]   = (*lBuf).lbId[  i+1];
   }

/* slot this message in:  */
(*lBuf).lbId[MAILSLOTS-1]     = ra.newestLo ;
(*lBuf).lbslot[MAILSLOTS-1]     = ra.catSector;

putLog(lBuf, logNo);
return TRUE;
}

/************************************************************************/
/* noteMessage() slots message into current room         */
/************************************************************************/
noteMessage(lBuf, logNo)
struct logBuffer   *lBuf;
int         logNo;
{
int roomNo;
char ucname[NAMESIZE];

strcpy(ucname, ra.msgBuf.mbto);
sToUpper(ucname);

ra.logBuf.lbvisit[0]   = ++ra.newestLo;

if (ra.thisRoom != MAILROOM) {
   note2Message(ra.newestLo, ra.catSector);

   /* write it to disk:    */
   putRoom(ra.thisRoom, &ra.roomBuf);
   noteRoom();
   } 
else {
   if (hash(ucname) != hash("SYSOP"))  {
      
      if (logNo != ra.thisLog)  {
         noteLogMessage(lBuf, logNo);      /* note in recipient    */
      }
      
      noteLogMessage(&ra.logBuf, ra.thisLog);      /* note in ourself      */
      fillMailRoom();            /* update room also     */
      }    
/*   else  */
     if(sysopflag) {
      getRoom(AIDEROOM, &ra.roomBuf);

      /* enter in Aide> room -- 'sysop' is special */
      note2Message(ra.newestLo, ra.catSector);

      /* write it to disk:       */
      putRoom(AIDEROOM, &ra.roomBuf);
      noteRoom();

      getRoom(MAILROOM, &ra.roomBuf);
      /* note in ourself if logged in: */
      if (ra.loggedIn)   noteLogMessage(&ra.logBuf, ra.thisLog);
      fillMailRoom();
      }
   }

/* make message official: */
ra.catSector  = ra.thisSector;
ra.catChar = ra.thisChar;
setUp(FALSE); 
return TRUE;
}

/************************************************************************/
/* note2Message() makes slot in current room... called by noteMess */
/************************************************************************/
note2Message(id, loc)
unsigned int loc;
unsigned long id;
{
int i;

/* store into current room: */
/* slide message pointers down to make room for this one:      */
for (i=0;  i<MSGSPERRM-1;  i++) {
   ra.roomBuf.msg[i].rbmsgLoc  = ra.roomBuf.msg[i+1].rbmsgLoc;
   ra.roomBuf.msg[i].rbmsgNo   = ra.roomBuf.msg[i+1].rbmsgNo ;
   }

/* slot this message in:      */
ra.roomBuf.msg[MSGSPERRM-1].rbmsgNo     = id ;
 if (ra.twit
   &&
   ra.thisRoom != MAILROOM
   &&
   ra.thisRoom != LOGROOM
   ) loc |= 0x8000; 
ra.roomBuf.msg[MSGSPERRM-1].rbmsgLoc    = loc;
return TRUE;
}


/************************************************************************/
/* printMessage() prints indicated message on modem & console  */
/************************************************************************/
printMessage(loc, id)
unsigned int loc;      /* sector in message.buf      */
unsigned long int id;        /* unique-for-some-time ID#     */
{
char dGetWord(), mAbort();
char c, moreFollows;
char lastc;
unsigned long int  hereLo;
int nCharOut, nearLineEnd;
struct logBuffer lBuf;
int  h, i, foundIt, logNo;



startAt((loc & 0x7FFF), 0);


do{
   getMessage();
/*   sscanf(ra.msgBuf.mbId, "%lu", &hereLo); */
   hereLo = (unsigned long) atol(&ra.msgBuf.mbId);
   }
while (hereLo != id &&  ra.thisSector ==  (loc & 0x7FFF)   );

if (hereLo != id  &&  !ra.usingWCprotocol) {
   mPrintf("\n Skipping Unavailable Message...\n ");
   cprintf("%lu\n\r", id);  /* for Sysop info */
   return;
   }

if (ra.ansi_on) doANSI(ra.ANSI_header);

 if(loc & 0x8000)  {
/* let's see if the author is !no twit to !unHOLD msg */
    normalizeString(ra.msgBuf.mbauth);
    h = hash(ra.msgBuf.mbauth);
    for (foundIt=i=0;  i<MAXLOGTAB && !foundIt;  i++) {
      if (ra.logTab[i].ltnmhash == h) {
      foundIt = TRUE;
      lseek(ra.logfl, (long int) ra.logTab[i].ltlogSlot*SECSPERLOG, SEEK_SET);
      if(_read(ra.logfl, &lBuf, SECSPERLOG) == SECSPERLOG)
        {

         if(!(lBuf.lbflags & TWIT)){
           ra.twited = FALSE;
           /* this message does not belong to a twit no more */
           ra.roomBuf.msg[ra.gloc].rbmsgLoc &= 0x7FFF;
         }
        else
            ra.twited = TRUE;
      }
      else cprintf("Log Read Error");
      }

   }

}


if (ra.twited && !( (strCmpU(ra.msgBuf.mbauth,ra.logBuf.lbname)==SAMESTRING)
   ||
   ra.aide ) ) {
      mPrintf("\n Skipping Unavailable Message...\n ");
      return;
      }
      

if (!ra.usingWCprotocol) {
   doCR();
   if (ra.twited           )      mPrintf(    "HELD"                 );
   if (!(ra.roomBuf.rbflags & ANON)){

      if (ra.msgBuf.mbdate[ 0])
      {
         sprintf(format,   "   %s",      ra.msgBuf.mbdate );
         mPrintf(format);
      }

      if (ra.msgBuf.mbto[   0])
      {
         sprintf(format,   "to %s ",     ra.msgBuf.mbto   );
         mPrintf(format);
      }

      if (ra.ansi_on) doANSI(ra.ANSI_author);

      if (ra.msgBuf.mbauth[ 0])
      {
         sprintf(format,   "from %s ",   ra.msgBuf.mbauth );
         mPrintf(format);
      }

      }
   else mPrintf(">-<");
   /* if (ra.msgBuf.mboname[0])      mPrintf(   " @%s",       ra.msgBuf.mboname);
*/
   /*if (ra.thisRoom != LOGROOM)*/ doCR();

      if (ra.ansi_on) doANSI(ra.ANSI_text);
      if (ra.msgBuf.mbHeadline[0] && ra.use_headline) {
         doCR();
         sprintf(format, "Headline: %s \n ", ra.msgBuf.mbHeadline);
         mPrintf(format);
      }   
      do {
         moreFollows     = dGetWord(ra.msgBuf.mbtext, 150);
         putWord(ra.msgBuf.mbtext);
      }
      while (moreFollows  &&  !mAbort());

     doCR();
     if (ra.outFlag != OUTSKIP && (ra.pause_at & 0x80) )
        doCharPause(1);
     mAbort(); /* set flags */

   }
else {
   /* networking dump of message: */
      /* fill in local node in origin fields if local message: */
      /* if (!ra.msgBuf.mborig[ 0])  strcpy(ra.msgBuf.mborig,  ra.nodeId      );
   if (!ra.msgBuf.mboname[0])  strcpy(ra.msgBuf.mboname, ra.nodeName   );
   if (!ra.msgBuf.mbsrcId[0])  strcpy(ra.msgBuf.mbsrcId, ra.msgBuf.mbId);
*/
      /* send header fields out: */
      /* if (ra.msgBuf.mbauth[ 0])  mWCprintf("A%s", ra.msgBuf.mbauth );
   if (ra.msgBuf.mb.date[ 0])  mWCprintf("D%s", ra.msgBuf.ra.date );
   if (ra.msgBuf.mboname[0])  mWCprintf("N%s", ra.msgBuf.mboname);
   if (ra.msgBuf.mborig[ 0])  mWCprintf("O%s", ra.msgBuf.mborig );
   if (ra.msgBuf.mbroom[ 0])  mWCprintf("R%s", ra.msgBuf.mbroom );
   if (ra.msgBuf.mbsrcId[0])  mWCprintf("S%s", ra.msgBuf.mbsrcId);
   if (ra.msgBuf.mbto[   0])  mWCprintf("T%s", ra.msgBuf.mbto   );

   #* send message text proper: *#
   sendWCChar('M');
*/
      if (ra.twited || ra.roomBuf.rbflags & ANON) return;
      if (ra.msgBuf.mbdate[ 0])  mWCprintf("\r\n    %s", ra.msgBuf.mbdate );
      if (ra.msgBuf.mbto[   0])  mWCprintf("to %s\r\n    ", ra.msgBuf.mbto);
      if (ra.msgBuf.mbauth[ 0])  mWCprintf("from %s ", ra.msgBuf.mbauth );
      if (ra.msgBuf.mbHeadline[0] && ra.use_headline) {
         doCR();
         mWCprintf("Headline: %s", ra.msgBuf.mbHeadline);
         doCR();
      }    
      mWCprintf("@ %s", ra.nodeName);
      mWCprintf(" %s\n\r  ", ra.nodeId );
/* if (ra.msgBuf.mbroom[ 0])  mWCprintf(" in %s> \r\n ", ra.msgBuf.mbroom );
   if (ra.msgBuf.mbsrcId[0])  mWCprintf("S%s", ra.msgBuf.mbsrcId);
*/

/*   doCR();*/
   lastc = ' ';
   nCharOut = 0;
   nearLineEnd = ra.termWidth * 8 / 10;

   do {
      c = getMsgChar();

      if ( (lastc=='\n')&&( (c==' ')||(c=='\011') ) )
      {
         doCR();
         nCharOut = 0;
      }

      if ( (nCharOut>nearLineEnd)&&((c==' ')||(c=='\n')||(c=='\011')) )
      {
         doCR();
         c='\025';
         nCharOut = 0;
      }

      lastc = c;
      if (c=='\n')  c=' ';
      if (c && c != '\025')
      {
/*         sendWCChar(c);*/
        if(c > 8) {
         _write(upld, &c, 1);
         nCharOut++;
       }  
      }
   }
   while (c);

   doCR();
 }
 
return TRUE;
}

/************************************************************************/
/* pullIt() is a sysop special to remove a message from a room */
/************************************************************************/
pullIt(m)
int m;
{
int i;

/* confirm that we're removing the right one:  */
ra.outFlag = OUTOK;
ra.gloc = m;
printMessage(ra.roomBuf.msg[m].rbmsgLoc, ra.roomBuf.msg[m].rbmsgNo);
if (!getYesNo("pull")) return;

/* record vital statistics for possible insertion elsewhere: */
ra.pulledMLoc = ra.roomBuf.msg[m].rbmsgLoc;
ra.pulledMId  = ra.roomBuf.msg[m].rbmsgNo ;

if (ra.thisRoom == AIDEROOM) return;

/* return emptied slot: */
for (i=m;  i>0;  i--) {
   ra.roomBuf.msg[i].rbmsgLoc = ra.roomBuf.msg[i-1].rbmsgLoc;
   ra.roomBuf.msg[i].rbmsgNo  = ra.roomBuf.msg[i-1].rbmsgNo ;
   }
ra.roomBuf.msg[0].rbmsgNo = 0 /*ERROR*/;   /* mark new slot at end as free */

/* store revised room to disk before we forget... */
noteRoom();
putRoom(ra.thisRoom, &ra.roomBuf);

/* note in Aide>: */
if (ra.whichIO == MODEM)  {
   sPrintf(ra.msgBuf.mbtext, "%s-",ra.logBuf.lbname);
   aideMessage( /* noteDeletedMessage== */ TRUE);

   }
return TRUE;
}

/************************************************************************/
/* putMessage() stores a message to disk           */
/* Always called before noteMessage() -- ra.newestLo not ++ed yet.   */
/* Returns: TRUE on successful save, else FALSE       */
/************************************************************************/
char putMessage(uploading)
char uploading;   /* true to get text via WC modem input, not RAM */
{
char *s, allOk;
int  putWCChar();

startAt(ra.catSector, ra.catChar);    /* tell putMsgChar where to write   */
putMsgChar(0xFF);          /* start-of-message       */

/* write message ID */
sprintf(format, "%lu", ra.newestLo+1);
dPrintf(format);
putMsgChar(0);

/* write date:   */
/*
    getDaTime();
    if(ra.year==90){
      dPrintf("D%2d:%02d %s %d%s%d ",
   ra.hour,
   ra.minute,
   dayTab[dayT],
   ra.date,
   ra.monthTab[ra.month],
   ra.year    );
      putMsgChar(0);
    }
*/

getDaTime();

sprintf(format, "D%2d|%02d|%02d %02d:%02d ",
   ra.year,
   ra.month,
   ra.date,
   ra.hour,
   ra.minute         );

dPrintf(format);
putMsgChar(0);

/* write room name out:    */
/*    dPrintf("R%s", ra.roomBuf.rbname);
    putMsgChar(0);
*/

if (ra.loggedIn) {
   /* write author's name out:       */
   sprintf(format, "A%s", ra.msgBuf.mbauth);
   }
   else
   {
   strcpy(format, "ACopperCit");
   }
   dPrintf(format);
   putMsgChar(0);     /* null to end string   */
   

if (ra.msgBuf.mbto[0]) {  /* private message -- write addressee  */
   sprintf(format, "T%s", ra.msgBuf.mbto);
   dPrintf(format);
   putMsgChar(0);
   }

if (ra.msgBuf.mbHeadline[0] && ra.use_headline) {
   sprintf(format, "H%s", ra.msgBuf.mbHeadline);
   dPrintf(format);
   putMsgChar(0);
}
   
/* write message text by hand because it would overrun dPrintf buffer: */
putMsgChar('M'); /* M-for-message. */
if (!uploading) {
   for (s=ra.msgBuf.mbtext;  *s;  s++) putMsgChar(*s);
   allOk = TRUE;
   } 
else {
   ra.outFlag = FALSE;    /* setup for putWCChar() */
   allOk = putWCChar();
   }

   if (allOk) {
      putMsgChar(0);     /* null to end text    */

      flushMsgBuf();
      }    
   else {
      flushMsgBuf();     /* so message count is ok */

    /* erase start-of-message indicator: */
    startAt(ra.catSector, ra.catChar);
      putmsgChar(0);     /* overwrite 0xFF byte */
      }

return  allOk;
}

/************************************************************************/
/* putMsgChar() writes successive message chars to disk     */
/* Globals: ra.thisChar=   ra.thisSector=       */
/* Returns: ERROR if problems else TRUE         */
/************************************************************************/
int putMsgChar(c)
char c;
{
char visible();
int  toReturn;
unsigned long tmp;

toReturn = TRUE;

/*#ifdef XYZZY
    if (ra.debug) cprintf("%c",visible(c));
#endif
*/
if (ra.sectBuf[ra.thisChar] == 0xFF)  {
   /* obliterating a msg   */
   ra.logBuf.lbvisit[(MAXVISIT-1)]  = ++ra.oldestLo;
   }

ra.sectBuf[ra.thisChar]   = c;

ra.thisChar   = ++ra.thisChar % SECTSIZE;

if (ra.thisChar == 0) {   /* time to write sector out a get next: */
   tmp = (unsigned long int) ra.thisSector;
   tmp *= SECTSIZE;
   lseek(ra.msgfl, tmp, SEEK_SET);
   if (_write(ra.msgfl, ra.sectBuf, SECTSIZE) != SECTSIZE) {
      cprintf("Msg File Write Error?");
      toReturn   = ERROR;
      }

   ra.thisSector  = (++ra.thisSector) % ra.maxMSector;
   tmp = (unsigned long) ra.thisSector;
   tmp *= SECTSIZE;
   lseek(ra.msgfl, tmp, SEEK_SET);
   if (_read(ra.msgfl, ra.sectBuf, SECTSIZE) >= 1000) {
      cprintf("Message File Read Error");
      toReturn   = ERROR;
      }
   }
return  toReturn;
}

/*----------------------------------------------------------------------*/
/* 30DEC86 mod for tabs by .bp.                 */
/* #putWord() - Writes one word to modem & console       */
/* NOTE: changed the lines that compare current column from '>'    */
/* to '>=' to fix the "One less than actuall width bug".    */
/*----------------------------------------------------------------------*/
putWord(st)
char *st;
{
char *s;
int newColumn;

for( newColumn=ra.crtColumn, s=st; *s; s++)
   {
   if( *s !=TAB)  ++newColumn;
   else     while( ++newColumn %8);   /* Expand the TAB */
   }

   if( newColumn >= ra.termWidth){
      /* BS to remove the trailing space */
      /* oChar(8);   
      */    doCR();
      }

/* Now for the actual send out the word routine */

for(; *st; st++)
   {
   if (ra.crtColumn >= ra.termWidth)  doCR();

   if( ra.prevChar != NEWLINE || (*st > ' ') ) putIt(*st);
   else {
      /* end of paragraph: */
    if( ra.outFlag == OUTPARAGRAPH)  ra.outFlag = OUTOK;
      doCR();
      putIt(*st);
      }
   }
return TRUE;
}

/*****************************************************************************/
/* putIt() to simplify tabs   .bp.                   */
/*****************************************************************************/
putIt(c)
char c;
{

if (c != TAB){
   oChar(c);
   ++ra.crtColumn;
   } 
else {
   if (!ra.termTab) {
      while( (++ra.crtColumn %8) && (ra.crtColumn != ra.termWidth) )
         oChar(' ');
      }
   else {
      while(++ra.crtColumn %8);
      oChar(TAB);
      }
   } 

   return TRUE;
}

/************************************************************************/
/*   showMessages() is routine to print roomful of msgs     */
/************************************************************************/
showMessages(whichMess, revOrder)
char whichMess, revOrder;
{
char iChar();
char c, once, xtmp[6];
int i, xsize=128;
int start, finish, increment;
unsigned long int lowLim, highLim, msgNo, preM;
char WCsave;

xtmp[0] = 0;

/* there is an error that permits ra.usingWCprotocol to be set
       FALSE during setUp */

ra.twited=FALSE;
once=TRUE;
WCsave = ra.usingWCprotocol;
setUp(FALSE);
ra.usingWCprotocol = WCsave;

/* Allow for reverse retrieval: */
if (!revOrder) {
   if (whichMess == LASTFIVE) start = MSGSPERRM - 4;
   else   start     = 0;
   finish       = MSGSPERRM;
   increment   = 1;
   }
else
 {
   start     = (MSGSPERRM -1);
   finish       = -1;
   increment   = -1;
   }

   switch (whichMess)   {
   case NEWoNLY:
      lowLim   = ra.logBuf.lbvisit[ ra.logBuf.lbgen[ra.thisRoom] & CALLMASK]+1;
      highLim = ra.newestLo;
      break;
   case OLDaNDnEW:
   case LASTFIVE:
      lowLim   = ra.oldestLo;
      highLim = ra.newestLo;
      break;
   case OLDoNLY:
      lowLim   = ra.oldestLo;
      highLim = ra.logBuf.lbvisit[ ra.logBuf.lbgen[ra.thisRoom] & CALLMASK];
      break;
   }

/* stuff may have scrolled off system unseen, so: */
/* was "if (lowLim < ra.oldestLo)...", rigged for wraparound: */
if (ra.oldestLo >= lowLim){
   lowLim = ra.oldestLo;
   }

/* if (!ra.expert && !ra.usingWCprotocol)  {
   tutorial("0.blb");
   }
*/
if(ra.usingWCprotocol) {
   setSpace(ra.homeDisk, ra.homeUser);
   
   upld = open(  tmpnam(pc), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);

   if (upld == -1) {
      ra.usingWCprotocol = FALSE;
      mPrintf("\n Message Download Error - Aborted...\n ");
      return(FALSE);
   }

   ra.usingWCprotocol = FALSE;
   mPrintf("\n Collecting Messages \n");
   mPrintf("\n Please Wait... \n ");
   ra.usingWCprotocol = TRUE;
}

for (i=start;   i!=finish;    i+=increment) {
   /* "<" comparison with 64K wraparound in mind: */
   msgNo = ra.roomBuf.msg[i].rbmsgNo;
   if (
      msgNo >= lowLim
      &&
      highLim >= msgNo
      ) {
      if( once
    &&
    i
    &&
    preM
    &&
    whichMess==NEWoNLY
    &&
    !ra.usingWCprotocol
    &&
    ra.lasto /* for default 1old del line */
    &&
    ra.thisRoom != MAILROOM
    &&
    (ra.oldestLo < preM)
    ){
      if(ra.roomBuf.msg[i-1].rbmsgLoc > 0x7FFF) ra.twited = TRUE;
      ra.gloc = i-1;
      printMessage(ra.roomBuf.msg[i-1].rbmsgLoc, preM);
      once=ra.twited=FALSE;
    }
      if (ra.outFlag) {
         if (  ra.outFlag == OUTNEXT || ra.outFlag == OUTPARAGRAPH)
            ra.outFlag = OUTOK;
         else
             if (ra.outFlag == OUTSKIP)   {
                  ra.echo = BOTH;
                  return;
              }
       }

    if(ra.roomBuf.msg[i].rbmsgLoc > 0x7FFF) ra.twited = TRUE;
    ra.gloc = i;
    printMessage(ra.roomBuf.msg[i].rbmsgLoc, msgNo);
    once=ra.twited=FALSE;
      /*   Pull current message from room if flag set */
    if (ra.pullMessage) {
       ra.pullMessage = FALSE;

       if (ra.aide || strCmpU(ra.logBuf.lbname, ra.msgBuf.mbauth)==SAMESTRING){
          pullIt(i);
          if (revOrder)  i++;
       }
    }

    if (!ra.usingWCprotocol && ra.thisRoom  == MAILROOM  &&
        whichMess == NEWoNLY &&
        !strCmpU(ra.logBuf.lbname, ra.msgBuf.mbauth)==SAMESTRING &&
        getYesNo("Reply")) {
              ra.reply = TRUE;
              if (makeMessage( /* uploading== */ FALSE)) i--;
              ra.reply = FALSE;
        }
    }
   preM=msgNo;
   };

if (ra.usingWCprotocol) {
/*   while(sendWCChar(ERROR) == FALSE);*/
  close(upld);
  xsendfile(pc, xsize);
  unlink(pc);
   }

upld = NULL;
   
return TRUE;
}

/************************************************************************/
/* startAt() sets location to begin reading message from    */
/************************************************************************/
startAt(sect, byt)
unsigned int sect;
unsigned int byt;
{
unsigned long tmp;
ra.GMCCache  = '\0';   /* cache to unGetMsgChar() into */

if (sect >= ra.maxMSector) {
   cprintf("Sector Calc Error");
   return;
   }
ra.thisChar   = byt;
ra.thisSector = sect;

tmp = (unsigned long) sect;
tmp *= SECTSIZE;
lseek(ra.msgfl, tmp, SEEK_SET);
if (_read(ra.msgfl, ra.sectBuf, SECTSIZE) >= 5000) {
   cprintf("Sector Read Error");
   }
return TRUE;
}

/************************************************************************/
/* unGetMsgChar() returns (at most one) char to getMsgChar()   */
/************************************************************************/
unGetMsgChar(c)
char c;
{
ra.GMCCache   = c;
return TRUE;
}

