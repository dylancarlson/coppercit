/************************************************************************/
/* Borland C++ 2.0  8/92                                        */
/* bds1.46   9/89       log.c          */
/*                         */
/*    userlog code for  Citadel bulletin board system    */
/************************************************************************/

/************************************************************************/
/*          history           */
/* 92Aug05 JCL  Port to MSDOS                   */
/* 86Jan30 BP   keep track of callers with logMsg()         */
/* 86Apr24 BP,MM,KK formating                */
/* 83Jun13 BAK  Modified terminate to handle ra.rcpm flag correctly. */
/* 83Jun11 BAK  Fixed phantom configuration bug.         */
/* 83Feb27 CrT Fixed login-in-Mail> bug.           */
/* 83Feb26 CrT Limited # new messages for new users.        */
/* 83Feb18 CrT Null pw problem fixed.              */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov03 CrT Began local history file & general V1.2 cleanup    */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*          contents          */
/*                         */
/* configure()    sets terminal parameters via dialogue  */
/* crypte()    encrypts/decrypts data blocks    */
/* doCR()         newline on modem and console     */
/* download()     menu-level routine for WC-protocol sends*/
/* getDfr()    for space on disk       */
/* getLog()    loads requested userlog record      */
/* hash()         hashes a string to an integer    */
/* login()     is menu-level routine to log caller in */
/* logMsg()    to keep track of callers      */
/* newPW()     is menu-level routine to change a PW   */
/* newUser()      menu-level routine to log a new caller */
/* noteLog()      enters a userlog record into RAM index */
/* printDate()    prints out ra.date         */
/* putLog()    stores a ra.logBuffer into ctdllog.sys */
/* PWSlot()    returns userlog.buf slot password is in */
/* slideLTab()    support routine for sorting ra.logTab  */
/* storeLog()                    */
/* strCmpU()      strcmp(), but ignoring case distinctions*/
/* tutorial()     prints a .hlp file         */
/* terminate()    menu-level routine to exit system   */
/* upload()    menu-level read-via-WC-protocol fn  */
/* userLog()      for list of users          */
/************************************************************************/
extern char getYesNo();
extern char mIready();

static int  bps[] = {300, 1200, 2400, 4800, 9600, 14400};

/************************************************************************/
/* configure() sets up terminal width etc via dialogue      */
/************************************************************************/
configure() {

void config_menu();

char abt = ' ', new_name[NAMESIZE];
int h,i, good;

pause_override = TRUE;

config_menu();

while( abt != 'X' && (carrDet() || ra.onConsole)){
  mPrintf("Selection: ");
  
  abt = (char) toupper(iChar());
  
  switch(abt) {
   case 'A' :
      if (!ra.expert) tutorial("wide.blb");
      ra.termWidth   = getNumber(" Terminal Width",         15, 132);
      --ra.termWidth;
      break;
   case 'B':   
      if (!ra.expert) tutorial("lineht.blb");
      ra.logBuf.pause_at = (ra.logBuf.pause_at & 0x80) |
        ((char) getNumber ("Screen Height in Lines (11-45): ", 11, 45) & 0x7f);
      break;
   case 'C':   
      if (!ra.expert) tutorial("lf.blb");
      sprintf(format, "\n Linefeed Added: %s\n ", 
              (ra.termLF ^= LFMASK) ? " Yes" : " No ");
      mPrintf(format);        
      break;
   case 'D':
      if (!ra.expert) tutorial("tab.blb");
      sprintf(format, "\n Tabs Sent: %s\n ", 
              (ra.termTab ^= TABMASK) ? " Yes" : " No");
      mPrintf(format);        
      break;
   case 'E':   
      if (!ra.expert) tutorial("lasto.blb");
      sprintf(format, "\n Show Last Old Message with New: %s \n  ", 
              (ra.lasto ^= LOMASK) ? " Yes " : " No");
      mPrintf(format);        
      break;
   case 'F':
      if (!ra.expert) tutorial("hlp.blb");
      sprintf(format, "\n Expert Mode On: %s\n ", 
              (ra.expert ^= EXPERT) ? " Yes " : " No");
      mPrintf(format);        
      break;
   case 'G':
      if (!ra.expert) tutorial("more.blb");
      sprintf(format, "\n Screen Pause is On: %s\n ", 
              (ra.termMore ^= MORE) ? " Yes" : " No");
      mPrintf(format);        
      if (ra.termMore) {
         if (getYesNo (" Pause Between Messages ") ) 
            ra.logBuf.pause_at |= 0x80;
         else
            ra.logBuf.pause_at &= 0x7f;
      }
      else ra.logBuf.pause_at &= 0x7f;         
      break;
   case 'H':
      if (!ra.expert) tutorial("pace.blb");
      sprintf(format, "\n Controlled Typeout is %s\n ",
             (ra.nopace ^= NOPACE) ? "Off" : "On");
      mPrintf(format);
      if (ra.nopace) ra.logBuf.lbnulls |= NOPACE;
      else ra.logBuf.lbnulls &=  ~NOPACE;
      break;
             
   case 'X': break;
   
   default: {
      if( !ra.loggedIn) return; 
      config_menu(); 
      break;
    }  
  }
}
storeLog();

pause_override = FALSE;

return TRUE;
}

/************************************************************************
 config_menu() - displays current configuration
************************************************************************/
void config_menu()
{
doCR();
mPrintf("User Configuration. Press Letter to Change\n ");
sprintf(format,"A) Terminal Width    : %d\n ", ra.termWidth);
mPrintf(format);
sprintf(format,"B) Screen Height     : %d\n ", ra.logBuf.pause_at & 0x7f);
mPrintf(format);
sprintf(format,"C) Add Linefeed to CR: %s\n ", (ra.termLF ? "Yes" : "No"));
mPrintf(format);
sprintf(format,"D) Expand Tabs       : %s\n ", (ra.termTab ? "Yes" : "No"));
mPrintf(format);
sprintf(format,"E) Last Old with New : %s\n ", (ra.lasto ? "Yes" : "No"));
mPrintf(format);
sprintf(format,"F) Expert Mode       : %s\n ", (ra.expert ? "Yes" : "No"));
mPrintf(format);
sprintf(format,"G) Screen Pause      : %s\n ", (ra.termMore ? "Yes" : "No"));
mPrintf(format);
if(ra.termMore) {
  sprintf(format,"     Pause After Each Msg  : %s\n ", (ra.logBuf.pause_at & 0x80
                                          ? "yes" : "No"));
  mPrintf(format);
}                                          
sprintf(format,"H) Controlled Typeout: %s\n ", (ra.nopace ? "No" : "Yes"));
mPrintf(format);
mPrintf("X) Exit Configuration\n ");
} 
/************************************************************************/
/* doCR() does a newline on modem and console         */
/************************************************************************/
doCR() {
int i;
char CR, LF, c[2], getMod_Peek();
int ch;

CR=13;
LF=10;

ra.crtColumn  = 1;
if (ra.outFlag) return;   /* output is being s(kip)ped  */


if (!ra.usingWCprotocol && !ansi_tut){
  putch('\r');
  putch('\n');

   if (p_port) {
      if(biosprint(0, '\r', p_port-1) & 0x29)
        { doStatusLine(S_MSGS,"\07 Check Printer \07 \n\r");
          p_port = 0;
        }
      if(biosprint(0, '\n', p_port-1) & 0x29)
        { doStatusLine(S_MSGS, "\07 Check Printer \07 \n\r");
          p_port = 0;
        }
   }
}
scrn++;
if(ra.termMore && !ra.usingWCprotocol && !pause_override){
   if (scrn > ( ra.pause_at & 0x7f) ) {
      scrn=0;

      doCharPause(1);

   }
}
else    if (scrn > ( ra.pause_at & 0x7f) ) scrn = 0;
   
if (ra.haveCarrier) {
   if (!ra.usingWCprotocol ) {
      outMod('\r');
      if (ra.termLF) outMod('\n');
       }
   else {
/*      sendWCChar('\r');
      if (ra.termLF) sendWCChar('\n');
*/
       _write(upld, &CR, 1);
       if (ra.termLF) _write(upld, &LF, 1);
      }
   }
ra.prevChar   = ' ';
return TRUE;
}

/************************************************************************/
/* download() is the menu-level send-message-via-WC-protocol fn   */
/************************************************************************/
download(whichMess, revOrder)
   char whichMess, revOrder;
{
ra.outFlag = OUTOK;
ra.echo    = NEITHER; 
ra.usingWCprotocol  = TRUE    ; /* all points bulletin     */
ra.SFRunning  = TRUE;     /* init sendWCChar coroutine  */

showMessages(whichMess, revOrder);

setUp(FALSE);
ra.usingWCprotocol  = FALSE  ;
return TRUE;
}

/***************************************************************/
/*   getDfr()                */
/*   27 feb 87 .bp. peeled from C. Forsberg's yam7.c of 1982  */
/*   This file assumes operation on a CP/M system    */
/***************************************************************/
/* return total free kilobytes of disk */

getDfr()
{
struct dfree free;

getdfree(0, &free);

ra.freeSpace =  ((unsigned long) free.df_avail) * ((unsigned long) free.df_bsec) * ((unsigned long) free.df_sclus);
ra.freeSpace = ra.freeSpace >> 10;

return TRUE;
}

/************************************************************************/
/* getLog() loads requested log record into RAM buffer      */
/************************************************************************/
getLog(lBuf, n)
struct logBuffer  *lBuf;
int         n;
{
unsigned long tmp;
tmp = (unsigned long) n;
if (lBuf == &ra.logBuf)   ra.thisLog  = n;

tmp *= SECSPERLOG;

lseek(ra.logfl, tmp, SEEK_SET);
if (_read(ra.logfl, lBuf, SECSPERLOG) != SECSPERLOG) {
   doStatusLine(S_MSGS, "Log Read Error");
   }
if (lBuf->pause_at == 0) lBuf->pause_at = 21;
return TRUE;
}

/************************************************************************/
/* hash() hashes a string to an integer            */
/************************************************************************/
int hash(str)
char *str;
{
int  h, i, shift;

for (h=shift=0;  *str;  shift=(shift+1)&7, str++) {
   h ^= (i=toUpper(*str)) << shift;
   }
return h;
}


/************************************************************************/
/* login() is the menu-level routine to log someone in      */
/************************************************************************/
login(password, username)
char *password, *username;    /* TRUE if parameters follow    */
{

FILE *callfl;
int  i, j;
int  foundIt, ltentry, match, ourSlot;
char ss[15];

foundIt =   ((ltentry = PWSlot(password)) != ERROR);

getDaTime();

ra.lminute = ra.minute;
ra.lhour = ra.hour;

   if(ra.haveCarrier){
     sprintf(ss,"%ld", ra.baud);
     }
   else {
     strcpy(ss, "CONSOLE");
     ra.baud = -1;
   }

   getDaTime();

   sprintf(format, "%2d|%02d|%02d %02d:%02d ",
      ra.year,
      ra.month,
      ra.date,
      ra.hour,
      ra.minute         );
   sprintf(&ra.call_record.time_day,"%-18s", format);

   sPrintf(&ra.call_record.name_baud, "NOLOGIN@%-19s", ss);
   sPrintf(format, "%02d:%02d", ra.hour, ra.minute);
   
   doStatusLine( T_LOGON, format);
   
   doStatusLine( USER, "NOLOGIN");
   normalizeString(username);
   normalizeString(ra.logBuf.lbname);
   
if (foundIt && *password && 
               ( ra.use_username ? 
                 (strCmpU(username, ra.logBuf.lbname) == SAMESTRING) : 1 )) {
   if (hash(ra.logBuf.lbname) == hash(ra.sysopname)) {
      ra.grand_poobah = TRUE;
      mPrintf("\n Welcome! Oh Exalted One\n ");
      }
   else   {
      sprintf(format,"Hi %s !!", ra.logBuf.lbname);
      mPrintf(format);
   }
   
   doStatusLine(USER, &ra.logBuf.lbname);
   
   /* update userlog entries: */

   ra.loggedIn     = TRUE;
   setUp(TRUE);


   sPrintf(format, "%c%-19s@%-7s",
      (ra.twit ? '*' : ' '),
      ra.logBuf.lbname,
      ss);

   sprintf(&ra.call_record.name_baud,   "%28s", format);

/*   logMsg(); */
   ra.outFlag = OUTOK;
   tutorial("postbul.msg");
   doSystemMsgs();
   if (!ra.twit){
      showMessages(NEWoNLY, FALSE);
      did_read = TRUE;
      listRooms(/* doDull== */ !ra.expert);
      }
   else tutorial("held.msg");

   if(!ra.expert){
      doCharPause(1);
      tutorial("reconfig.msg");
   }
   
   if (( ra.logBuf.lbId[MAILSLOTS-1]    >=
      (ra.logBuf.lbvisit[   ra.logBuf.lbgen[MAILROOM] & CALLMASK   ]+1)) &&
      ra.logBuf.lbId[MAILSLOTS-1] >= ra.oldestLo && ra.thisRoom != MAILROOM)
         {
          tutorial("email.blb");
         }
   }
   else {
   /* discourage password-guessing: */
      if (strlen(password) > 2)
         {
          --ra.guess;
         }

   mPrintf("\n Unknown Identity ");

   if (ra.whichIO==MODEM && !ra.guess || !ra.unlogLoginOk)  {
      tutorial("BADPASS.BLB");

      sPrintf(&ra.call_record.name_baud, "NOLOGIN@%-19s", ss);
      strcpy(ra.msgBuf.mbtext, &ra.call_record);
      logMsg(); 

      terminate(TRUE)
      ;
      }
   else if (getYesNo("Are You A New user")) {
      if (!newUser()) return;
      sPrintf(&ra.call_record.name_baud, "%c%-19s@%-7s",
             (ra.twit ? '*' : ' '),
             ra.logBuf.lbname,
             ss);

      if(ra.loggedIn) {
         doSystemMsgs();
         if (!ra.twit){
            showMessages(NEWoNLY, FALSE);
            did_read = TRUE;
            listRooms(/* doDull== */ !ra.expert);
            }
         }
         else tutorial("held.msg");

           }
      }
if(ra.loggedIn) {
   ra.callerno += 1;
   sprintf(format, "\n \n You Are Caller #%ld\n ", ra.callerno);
   mPrintf(format);
   setSpace(ra.homeDisk, ra.homeUser);
   callfl = fopen("CALLNO.SYS", "w+t");
   fprintf(callfl, "%ld", ra.callerno);
   fclose(callfl);
   neverLoggedIn = FALSE;
}   
   return TRUE;
}

/************************************************************************/
/* logMsg() saves call record in Logroom>          */
/************************************************************************/
logMsg()
{
int ourRoom;
if(! ra.whichIO == MODEM) return;

/* message is already set up in ra.msgBuf.mbtext */
putRoom(ourRoom=ra.thisRoom, &ra.roomBuf);
getRoom(LOGROOM, &ra.roomBuf);
strcpy(ra.msgBuf.mbauth, "CopperCit") ;
ra.msgBuf.mbto[0] = '\0';
ra.msgBuf.mbHeadline[0]='\0';

if (putMessage( /* uploading== */ FALSE))   noteMessage(0, ERROR);

putRoom(LOGROOM, &ra.roomBuf);
noteRoom();
getRoom(ourRoom, &ra.roomBuf);

return TRUE;
}

/************************************************************************/
/* newPW() is menu-level routine to change one's password      */
/* since some Citadel nodes run in public locations, we avoid  */
/* displaying passwords on the console.            */
/************************************************************************/
newPW() {
char oldPw[NAMESIZE];
char pw[NAMESIZE];
char *s;
int i, goodPW;

/* save password so we can find current user again: */
strcpy(oldPw, ra.logBuf.lbpw);
storeLog();

   do {
       getString("New Password ", pw, NAMESIZE);

       if (strlen(pw)) { 
           normalizeString(pw);

          /* check that PW isn't already claimed: */
          goodPW = (PWSlot(pw) == ERROR  &&  strlen(pw) >= 4);

          if (!goodPW) mPrintf("Bad Password\n ");
       }
       else goodPW = TRUE;
      } 
   while (!goodPW && (ra.haveCarrier    || ra.whichIO==CONSOLE) );

   doCR();
   PWSlot(oldPw);         /* reload old log entry        */

if (goodPW) { /* accept new PW:        */
   strcpy(ra.logBuf.lbpw, pw);
   ra.logTab[0].ltpwhash   = hash(pw);
   storeLog();
   }

sprintf(format,"%s\n Password: ", ra.logBuf.lbname);
mPrintf(format);
if (ra.whichIO == CONSOLE) {
   cprintf( "%s\r\n ", ra.logBuf.lbpw);
   } 
else {
   s  = ra.logBuf.lbpw;
   while (*s)    outMod(*s++);
   doCR();
   }
return TRUE;
}

/************************************************************************/
/* newUser() prompts for name and password         */
/************************************************************************/
newUser() {

   FILE *newfolx;
   char ss[15];
   char fullnm[NAMESIZE], real_name[40];
   char pw[NAMESIZE], eve_phone[20];
   char *s;
   int  good, h, i, ourSlot, lowSlot;
   unsigned long low_msg = 0x7FFFFFF;
   timeout += 180;   /* give the newuser time to do the questionaire */

   if (ra.twitbit)
      gotoRoom("Mail");
   else
      gotoRoom("Lobby");
         
   ra.expert = FALSE;
   
   tutorial("name.blb");

   /* get name and check for uniqueness... */
   do {
      doCR();
      getString("Name", fullnm, NAMESIZE);
      normalizeString(fullnm);
      h  = hash(fullnm);
      for (i=0, good=TRUE;   i<MAXLOGTAB && good;   i++) {
  
      if (h == ra.logTab[i].ltnmhash) good = FALSE;
      }

      if (!h || h==hash("Citadel") || h==hash("Sysop") || strlen(fullnm) < 3){
         good = FALSE;
      }
      
      /* lie sometimes -- hash collision !=> name collision */
      if (!good){
         sprintf(format,"%s inuse", fullnm);
         mPrintf(format);
      }
   }    
   while (!good &&  (ra.haveCarrier || ra.whichIO==CONSOLE));

   /* get password and check for uniqueness...  */

   tutorial("password.blb");

   do {
      mPrintf("Password: ");
      getString("",  pw, NAMESIZE);
      normalizeString(pw);

      h  = hash(pw);
      for (i=0, good=strlen(pw) > 3;  i<MAXLOGTAB  &&  good;  i++) {
         if (h == ra.logTab[i].ltpwhash) good = FALSE;
      }
      
      if (!h)   good = FALSE;
      
      if (!good) {
         mPrintf("Bad Password\n ");
      }
   }    
   while(!good && (ra.haveCarrier || ra.whichIO==CONSOLE));

   getString("Real Name " , real_name, 38);
   getString("Evening Phone Number " , eve_phone, 18);
   sprintf(format,"\n User Name %s \n Password: ", fullnm);
   mPrintf(format);
   
   if (ra.whichIO == CONSOLE) {
      cprintf("%s", pw);
      }    
   else {
      sprintf(format,"%s\n ", pw);
      mPrintf(format);
      }
   sprintf(format, "\n Real Name: %s\n Evening Phone Number: %s\n ", 
           real_name, eve_phone);
   mPrintf(format);
   if( !(getYesNo("OK") && (carrDet() || ra.whichIO==CONSOLE)) ) 
     return (FALSE);

   setSpace(ra.homeDisk, ra.homeUser);
   newfolx = fopen("NEWFOLX.SYS", "a+t" );
   sprintf(format,"%s %s %s\n", fullnm, real_name, eve_phone);
   fputs(format, newfolx);
/*   fflush(newfolx);*/
   fclose(newfolx);
   strcpy(&ra.msgBuf.mbtext, "Newuser: ");
   strcat(&ra.msgBuf.mbtext, format);
   logMsg();
   
   
   doStatusLine(USER, fullnm);
   
   /* first look for empty slot then kick out least */
   /* recent twit caller then if no twits the least */
   /* recent caller from the userlog and claim entry:  */
   /*
   h=j=-1;
   for (i=0;i<MAXLOGTAB;i++){
   if (ra.logTab[i].ltnewest == ERROR){
      h=i;
      break;
   }
   }
   if (h!=-1) {
            ourSlot = ra.logTab[h].ltlogSlot;
       slideLTab(0,h);
   } else {
   for (i=MAXLOGTAB-1;i>0;i--){
      if (ra.logTab[i].ltnewest == 0){
         j=i;
         break;
      }
   }
   }
   if (j!=-1) {
            ourSlot = ra.logTab[j].ltlogSlot;
       slideLTab(0,j);
   } else {
       ourSlot = ra.logTab[MAXLOGTAB-1].ltlogSlot;
            slideLTab(0, MAXLOGTAB-1);
   }
*/
   /*    Peter T.'s fix */  
   h=-1; 

   /* 
   * Empty slot? 
   */ 
   for (i=0;i<MAXLOGTAB;i++) 
      { 
      if (ra.logTab[i].ltnewest == ERROR) 
         {
          h=i; 
          break; 
      }       
   }    

   /* 
   * No empty, search for twit.. 
   */ 
   if (h == -1)  
      { 
      for (i=MAXLOGTAB-1;i>0;i--) 
         { 
          if (ra.logTab[i].ltnewest == 0) 
             { 
              h=i; 
              break; 
          }   
      }       
   }    

   /* 
     * Ok, then axe someone..  
     */ 
   if (h == -1) 
      { 
      h = MAXLOGTAB-1; 
      }    

   ourSlot = ra.logTab[h].ltlogSlot; 
   slideLTab(0,h); 




   ra.logTab[0].ltlogSlot = ourSlot;
   getLog(&ra.logBuf, ourSlot);

   /* copy info into record:  */
   strcpy(ra.logBuf.lbname, fullnm);
   strcpy(ra.logBuf.lbpw, pw);
   /*  low = ra.newestLo-50;
   if (ra.oldestLo >= low)   low = ra.oldestLo;
 */
   for (i=1;  i<MAXVISIT;  i++)   ra.logBuf.lbvisit[i]=/*low*/ra.newestLo;
   ra.logBuf.lbvisit[               0]= ra.newestLo;
   ra.logBuf.lbvisit[         (MAXVISIT-1)]= ra.oldestLo;

   /* initialize rest of record: */
   for (i=0;  i<MAXROOMS;  i++) {
      if (ra.roomTab[i].rtflags & PUBLIC) {
         h = (ra.roomTab[i].rtgen);
         ra.logBuf.lbgen[i] = (h << GENSHIFT) + (MAXVISIT-1);
      }       
      else {
           /* set to one less */
           h = (ra.roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
           ra.logBuf.lbgen[i] = (h << GENSHIFT) + (MAXVISIT-1);
      }
   }
   
   for (i=0;  i<MAILSLOTS;  i++)  {
      ra.logBuf.lbslot[i] = 0;
      ra.logBuf.lbId[  i] = ra.oldestLo -1;
   }

   for (i = 0; i < 16; i++) ra.logBuf.BIO_waste[i] = 0;
   
   ra.expert = 0;
   
   /* fill in ra.logTab entries  */
   ra.logTab[0].ltpwhash   = hash(pw)     ;
   ra.logTab[0].ltnmhash   = hash(fullnm)    ;
   ra.logTab[0].ltlogSlot  = ra.thisLog      ;
   ra.logTab[0].ltnewest   = ra.logBuf.lbvisit[0];

   /* special kludge for Mail> room, to signal no new mail:   */
   ra.roomTab[MAILROOM].rtlastMessage = ra.logBuf.lbId[MAILSLOTS-1];

   sprintf(format," %s \n ",ra.logBuf.lbname);
   mPrintf(format);

   if(ra.twitbit) ra.twit     = TWIT;
   else ra.twit = 0;
   ra.loggedIn = TRUE;

   tutorial("newconfg.blb");
   
   ra.logBuf.pause_at = ra.pause_at;
   
   if (!getYesNo("This default OK ")) { 
      configure(); 
   } 
   

   storeLog();

/*   if(ra.baud > B9600) strcpy(ss, "HISPEED");
   else */
   ltoa(ra.baud, ss, 10);

   sPrintf(ra.msgBuf.mbtext, "*%15s@%-9s", ra.logBuf.lbname, ss);
   
  /* logMsg();*/

   tutorial("postbul.msg");
   doCharPause(1);
   tutorial("newu.blb");
   doCharPause(1);
   return TRUE;
   }

/************************************************************************/
/* noteLog() notes logTab entry in RAM buffer in master index  */
/************************************************************************/
noteLog() {
int i, j, slot;

/* figure out who it belongs between: */
for (i=0;  ra.logTab[i].ltnewest > ra.logBuf.lbvisit[0];   i++);

/* note location and open it up:      */
slot = i;
slideltab(slot, MAXLOGTAB-1);

/* insert new record */
ra.logTab[slot].ltnewest  = ra.logBuf.lbvisit[0]  ;
ra.logTab[slot].ltlogSlot = ra.thisLog        ;
ra.logTab[slot].ltpwhash  = hash(ra.logBuf.lbpw)  ;
ra.logTab[slot].ltnmhash  = hash(ra.logBuf.lbname);
return TRUE;
}

/************************************************************************/
/* printDate() prints out a date packed in two-byte format  */
/************************************************************************/
printDate()
{     

getDaTime();

sprintf(format, " %2d|%02d|%02d %02d:%02d ",
   ra.year,
   ra.month,
   ra.date,
   ra.hour,
   ra.minute      );
mPrintf(format);
return TRUE;
}

/************************************************************************/
/* putLog() stores givend log record into ctdllog.sys    */
/************************************************************************/
putLog(lBuf, n)
struct logBuffer  *lBuf;
int         n;
{
unsigned long tmp;
tmp = (unsigned long) n;
tmp *= SECSPERLOG;


lseek(ra.logfl, tmp, 0);
if (_write(ra.logfl, lBuf, SECSPERLOG) != SECSPERLOG) {
   doStatusLine(S_MSGS, "Log Write Error");
   }

return TRUE;
}

/************************************************************************/
/* PWSlot() returns userlog.buf slot password is in, else ERROR   */
/* NB: we also leave the record for the user in ra.logBuf.     */
/************************************************************************/
int PWSlot(pw)
char pw[NAMESIZE];
{
int  h, i;
int  foundIt, ourSlot;

h = hash(pw);

/* Check all passwords in memory: */
for(i=0, foundIt=FALSE;  !foundIt && i<MAXLOGTAB;  i++) {
   /* check for password match here */

   /* If password matches, check full password        */
   /* with current newUser code, password hash collisions should  */
   /* not be possible... but this is upward compatable & cheap */
   if (ra.logTab[i].ltpwhash == h) {
      ourSlot = ra.logTab[i].ltlogSlot;
      getLog(&ra.logBuf, ourSlot);

      if (strCmpU(pw, ra.logBuf.lbpw) == SAMESTRING) {
         /* found a complete match */
         ra.thisSlot = i   ;
         foundIt  = TRUE;
      }
   }
}

if (foundIt)   return ra.thisSlot;
else    return ERROR   ;
}

/************************************************************************/
/* slideLTab() slides bottom N lots in ra.logTab down.  For sorting. */
/************************************************************************/
slideLTab(slot, last)
   int slot;
int last;
{
int i, j;

/* open slot up: (movmem isn't guaranteed on overlaps) */
for (i=last-1;  i>=slot;  i--)  {
   movmem(&ra.logTab[i], &ra.logTab[i+1], ra.sizeLTentry);
   }
return TRUE;
}

/************************************************************************/
/* storeLog() stores the current log record.       */
/************************************************************************/
storeLog()  {

ra.logTab[0].ltnewest = /* ra.twit ? 0 : */ ra.newestLo;

ra.logBuf.lbvisit[0]  = /* ra.twit ? 0 : */ ra.newestLo;
ra.logBuf.lbwidth       = ra.termWidth;
ra.logBuf.lbflags       = ra.expert | ra.termUpper | ra.termLF
| ra.termTab | ra.aide | ra.lasto | ra.twit | ra.termMore;

if (ra.loggedIn)     putLog(&ra.logBuf, ra.thisLog);  /*<<<<added test */
return TRUE;
}

/************************************************************************/
/* strCmpU() is strcmp(), but ignoring case distinctions    */
/************************************************************************/
int strCmpU(s, t)
char s[], t[];
{
int  i;

i = 0;

while (toUpper(s[i]) == toUpper(t[i])) {
   if (s[i++] == '\0')  return 0;
   }
return  toUpper(s[i]) - toUpper(t[i]);
}

/************************************************************************/
/* terminate() is menu-level routine to exit system      */
/************************************************************************/
terminate(discon)
char discon;
/* 1.  parameter <discon> is TRUE or FALSE.     */
/* 2.  if <discon> is TRUE, breaks modem connection   */
/*     or switches ra.whichIO from CONSOLE to MODEM,  */
/*     as appropriate.              */
/* 3.  modifies externs: struct ra.logBuf,      */
/*        struct ra.logTab       */
/* 4.  returns no values            */
/*       modified dvm 9-82       */
{

char tmp[L_tmpnam], *call_rec, *reset;
int i;
FILE *j, *callfl;

ra.grand_poobah = FALSE;

if (discon && !is_door /* && carrDet() */){
   if (byeflag && ra.loggedIn) tutorial("byebye.blb");
   else byeflag = TRUE;
   
   flush_fossil();
   reset = ANSI_reset;
   if (ra.ansi_on) {
     while ( *reset != '\0') outMod(*reset++);
     outMod('\n');
   }  
   hangUp();
}

if( (ra.loggedIn )) {
  setSpace(ra.homeDisk, ra.homeUser);
  tmpnam(tmp);
  j = fopen(tmp, "w+t");
  callfl = fopen("CALLLOG.SYS", "r+t" );
  sPrintf(&ra.call_record.timeOn,"%10d min.%c%c", (int) timeOn(), '\015','\012');
  ra.call_record.lf = ra.call_record.cr = '\0';
  sprintf(format, "%s\n", &ra.call_record);
  fputs(format, j);
  i = 0;
  while ( i++ < 99 &&
          (fgets(format, 128, callfl) != NULL))
              fputs(format, j);
  fflush(j);            
  fclose(j);
  fclose(callfl);
  unlink("CALLLOG.SYS");            
  rename(tmp, "CALLLOG.SYS");
  }
else
 {
  if (neverLoggedIn) {
     sPrintf(&ra.call_record.name_baud, "NOLOGIN@%-19s", ra.baud_asc);
     strcpy(ra.msgBuf.mbtext, " Connect: ");
     strcat(ra.msgBuf.mbtext, &ra.call_record);
     logMsg();
     }
  }
  
if (ra.loggedIn) {
   ra.logBuf.lbgen[ra.thisRoom]  = ra.roomBuf.rbgen << GENSHIFT;
   storeLog();
   ra.loggedIn = FALSE;
   setUp(TRUE);
   }

  
if (discon)  {
   if(ra.whichIO == CONSOLE) doStatusLine(WHICH_IO, "  MODEM  ");
   ra.whichIO = MODEM;
   initStatusLine();
   if (is_door || is_mailer)  {
      ra.exitToCpm = TRUE;
      }    
   else {
      if(!ra.lockedport) 
        setbaud(ra.hibaud);
      initString();

    }
}

gotoRoom("LOBBY");
doStatusLine(S_MSGS, "Waiting...");
if (for_sysop && discon) {
   doOffhook();
   sysop_hold = (long) time(NULL) + 300; /*hold up to 5 minutes */
   while( (long) time(NULL) < sysop_hold  && !KBReady()) 
        if ( (time(NULL) % 10) == 0) {
           putch(BELL);
           delay(500);
           putch(BELL);
        }   
   doOnhook();
   for_sysop = FALSE;
   if ( KBReady() ) getCh();     
}       
 
if (discon) initPort();
return TRUE;

}

/************************************************************************/
/* tutorial() prints file <filename> on the modem & console */
/* Returns: TRUE on success else ERROR       */
/************************************************************************/
#define MAXWORD 256
tutorial(filename)
char *filename;
{
char tformat[MAXWORD],line[MAXWORD], holdit, hold_ansi, save_tdl;
int  toReturn;
FILE *fp;

sToUpper(filename);
toReturn   = TRUE;
save_tdl = ra.textDownload;
ra.textDownload = TRUE;
holdit = ra.termMore;
scrn = 0;
hold_ansi = ra.ansi_on;

ra.outFlag = OUTOK;
doCR();

setSpace(ra.homeDisk, ra.homeUser);

/* in case not all helps/etc are not ANSI, we'll look in the text
   directory also */
   
if(ra.ansi_on) {
  chdir(ra.ansi_dir);
  if((fp=fopen(filename, "r")) != NULL){
     fclose(fp);
     if (clr_screen) {
        outMod(SPECIAL);
        outMod('[');
        outMod('2');
        outMod('J');
      }  
    } 
  else {
     chdir(ra.text_dir);
     hold_ansi = FALSE;
    } 
  }
else 
  chdir(ra.text_dir);
  
   
if((fp = fopen(filename, "r" )) == NULL) {
   sprintf(tformat, "\n %s Not Found \n ", filename); 
   cprintf(tformat);
   toReturn = ERROR;
   } 
else {
   doCR();
   if (hold_ansi) {
      fclose(fp);
      if (strstr(filename, ".BLB") != NULL || strstr(filename, ".MNU") != NULL)
         ra.termMore = FALSE;
      ansi_tut = TRUE;   
      transmitFile(filename);
      ansi_tut = FALSE;
      if (clr_screen) {
        outMod(SPECIAL);
        outMod('[');
        outMod('2');
        outMod('2');
        outMod(';');
        outMod('1');
        outMod('H');
      }
   }
   else {   
   while ( (ra.outFlag!=OUTNEXT) &&(ra.outFlag!=OUTSKIP) &&
         (fgets(line, MAXWORD, fp /*fbuf*/)) &&( (ra.whichIO==CONSOLE) ||
         (carrDet() ) )) {
             if((line[0] == '@') && (char) toupper(line[1] == 'P')) {
               if (!ra.termMore) doCharPause(1);
               else {
                  sprintf(tformat, "%s", &line[2]);
                  mPrintf(tformat);
                  }
             }
             else {
               sprintf(tformat, "%s", line);
               mPrintf(tformat);
            }   

    }

   fClose(fp );
   doCR();
   }
 }

ra.textDownload = save_tdl;
clr_screen = TRUE;
ra.termMore = holdit;
scrn = 0;
setSpace(ra.homeDisk, ra.homeUser);
return   toReturn;
}

/************************************************************************/
/* upLoad() enters a file into current directory         */
/************************************************************************/
upLoad() {
char fileName[NAMESIZE], *s;
int putFlChar(), i;
void fileInfo();

if (!ra.expert) tutorial("file.blb");
getString("filename", fileName, NAMESIZE);
normalizeString(fileName);
sToUpper(fileName);

i=-1;

while( *DOS_devices[++i] != '\0'){
   if(!strcmp(DOS_devices[i], fileName) )
     {  mPrintf("\n Invalid Filename \n ");
        return;
     }    
}
   
if(strchr(fileName, ':') ||
   strchr(fileName, '\\') || strchr(fileName,' ') )
  { mPrintf("\n Invalid Character in Filename!! \n ");
    return;
  }  
s = &fileName[0];


   setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);

if((upld = open(s, O_RDONLY | O_BINARY)) != -1) {
   mPrintf("\n That file exists here\n\x07 ");
   close( upld /*ra.msgBuf.mbtext*/);  /* or loose file & blurbs  .bp. */
   return;
   } 
else {
   if (!ra.expert) {
      setSpace(ra.homeDisk, ra.homeUser);
      tutorial("wcupload.blb");
      setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);
      }
   getDfr();
   if ((!ra.freeSpace)||
      (( upld = open(s, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE)) == -1)){
         mPrintf("\n Full Disk or Bad Character in Filename");
         unlink(s);
   }    
   else {
      /* get space available */
      sprintf(format, "%lu kb Free:",ra.freeSpace);
      mPrintf(format);  
      
      /* then upload         */
      if(xreadFile( upld ))
        { 
        close( upld );
        fileInfo(fileName);
        setSpace (ra.homeDisk, ra.homeUser);
        sPrintf(ra.msgBuf.mbtext, "%s to %s by %s",
        fileName,
        ra.roomBuf.rbname,
        ra.logBuf.lbname);
        logMsg();
 /* now wait for 3 seconds or first character input, whichever is first */
        receive(3);   /* then */
    }       
else {
    close(upld );  /* or loose blurbs .bp. */
    unlink(s);
    }
   }
 }
setSpace(ra.homeDisk, ra.homeUser);
return TRUE;
}

/***********************************************************************/
/*     USERLOG() prints userlog -- maher 7-29-86  mod by bp            */
/***********************************************************************/
userLog() {  


   if(ra.loggedIn){ 
      mPrintf("\n ");
      strcpy(ra.msgBuf.mbsrcId, ra.logBuf.lbpw);
      storeLog();
      for (ra.thisLog=0; !ra.outFlag && ra.thisLog < MAXLOGTAB; ra.thisLog++) {
    if (mAbort()) break;
    getLog(&ra.logBuf, ra.thisLog);
    if (strlen(ra.logBuf.lbname) > 0
       &&
       (!(ra.logBuf.lbflags & TWIT) || ra.whichIO==CONSOLE)
       ) {
       sprintf(format, "\n %s",ra.logBuf.lbname);
       mPrintf(format);  
       cprintf("\t%s%s",
          ra.logBuf.lbflags & AIDE ? "A" : " ",
          ra.logBuf.lbflags & TWIT ? "T" : " "
          );
       }
    }
      PWSlot(ra.msgBuf.mbsrcId);
      }
   return TRUE;
   }


/*too big*/
/***********************************************************************/
/*     USERLOG() prints userlog           -- maher 7-29-86             */
/***********************************************************************/
/*
userlog() {  
   char oldPw[NAMESIZE];
if(ra.loggedIn) { 
   strcpy(oldPw, ra.logBuf.lbpw);
   storeLog();
   mPrintf("\n\n");
   for (ra.thisLog=0; ra.thisLog < MAXLOGTAB; ra.thisLog++) {
   getLog(&ra.logBuf, ra.thisLog);
   if (strlen(ra.logBuf.lbname) > 2) {
      mAbort();
      mPrintf("%-17s %3d %2s %2s %2s %2s %2s\n\n",
       ra.logBuf.lbname,ra.logBuf.lbwidth,
      (ra.logBuf.lbflags & AIDE    ? "AD" : " "),
      (ra.logBuf.lbflags & UCMASK  ? "UC" : " "),
      (ra.logBuf.lbflags & LFMASK  ? "LF" : " "),
      (ra.logBuf.lbflags & TABMASK ? "TB" : " "),
      (ra.logBuf.lbflags & EXPERT  ? "EX" : " "));     
               }
      
                     }
   PWSlot(oldPw);
   }
 }
*/

