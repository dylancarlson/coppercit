/************************************************************************/
/* Borland C++ 2.0  8/92                 */
/* bds1.46  9/89        rooma.c        */
/*    room code for Citadel bulletin board system     */
/************************************************************************/

/************************************************************************/
/*          history           */
/* 92Aug05 JCL Ported to MSDOS     */
/* 86April BP, MM, KK modified                  */
/* 83Feb24  Insert check for insufficient RAM, externs too low.   */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov05 CrT main() splits off to become citadel.c        */
/************************************************************************/

#include "ctdl.h"

/************************************************************************/
/*          Contents          */
/*                         */
/* dumpRoom()     tells us # new messages etc      */
/* fileDir()      prints out a filename for a dir listing */
/* fillMailRoom()    set up Mail> from log record     */
/* gotoRoom()     handles "g(oto)" command for menu   */
/* init()         system startup initialization    */
/* initCitadel()                    */
/* initSysop()                   */
/* listRooms()    lists known rooms       */
/* openFile()     opens a .sys file       */
/* readSysTab()      restores system state from citadel.tab */
/* roomExists()      returns slot# of named room else ERROR */
/* setSpace()     set default disk and user#    */
/* setUp()                    */
/* systat()    shows current system status      */
/* wildCard()     expands ambiguous filenames      */
/* writeSysTab()     saves state of system in citadel.tab   */
/*                         */
/************************************************************************/

char readSysTab();
char *sToUpper();
/************************************************************************
 
 doJump() "goto" room by room number
 
 ************************************************************************/
void doJump()

{
 extern char gotoRoom();
 char roomName[NAMESIZE];
 char roomNum[3];
 int  rmNum;
 
 mPrintf("\bJump to Room Number: ");
 getString("", roomNum, 3);
 rmNum = atoi(roomNum);
 
 if(rmNum > MAXROOMS || rmNum < 0) return;
 if(!(ra.roomTab[rmNum].rtflags & INUSE)) return;
 if(!(ra.roomTab[rmNum].rtflags & PUBLIC)) {
   mPrintf("\n \n You Cannot <J>ump to a Non-public Room\n ");
   mPrintf("Use <.G>oto COMPLETE_ROOM_NAME \n");
   return;
   }
 
 gotoRoom(ra.roomTab[rmNum].rtname);
 
}
   
/************************************************************************/
/* dumpRoom() tells us # new messages etc          */
/************************************************************************/
dumpRoom() {
int fileDir();
unsigned long int i, count, loc, newCount;
unsigned long int no, tmp;

for (newCount=0, count=0, i=0;   i<MSGSPERRM;   i++) {

   loc   = ra.roomBuf.msg[i].rbmsgLoc;
   no = ra.roomBuf.msg[i].rbmsgNo ;
   if (loc != FALSE /*ERROR*/) {

      if (no >= ra.oldestLo) count++;

      /* don't boggle -- just checking against ra.newestLo as of */
      /* the last time we were  in this room:         */

      tmp=(ra.logBuf.lbvisit[ ra.logBuf.lbgen[ra.thisRoom] & CALLMASK ]+1);
      
      if(ra.debug)
           cprintf("No: %lu  Tmp: %lu No>=Tmp %d\n\r", no, tmp, (no >= tmp));  

      if (no >= tmp)  newCount++;
      }
   }
/* lazy patch: */
if (newCount > count)   newCount = count;

if (ra.loggedIn && newCount)
   {  
   sprintf(format, "\n %lu msgs, %lu new", count, newCount);
   mPrintf(format);
   }
return TRUE;
}

/************************************************************************/
/* fileDir() prints out one filename and size, for a dir listing  */
/************************************************************************/
fileDir(fileName)
char *fileName;
{
int fd, i;        
long fsize;
int fday, fmonth, fyear;
struct ffblk fildata;

char tempName[NAMESIZE], tBuf[2*NAMESIZE];

for (i=0; i < NAMESIZE; i++) tempName[i] = ' ';
for (i=0; i < 2*NAMESIZE; i++) tBuf[i] = ' ';

ra.outFlag = OUTOK;

fildata.ff_fsize = 0l;

unspace(fileName, tempName);

findfirst(tempName, &fildata, 0 );

fday = fildata.ff_fdate & 0x0001f;
fmonth = (fildata.ff_fdate & 0x01e0) >> 5;
fyear = ((fildata.ff_fdate & 0XFE00) >> 9 ) + 80;

sprintf(tBuf, "%-12s %02d/%02d/%02d %9ld : ", 
        tempName, fmonth, fday, fyear, fildata.ff_fsize );

putWord(tBuf);

mAbort();      /* chance to next(!)/pause/skip */

return TRUE;
}

/************************************************************************/
/* fillMailRoom()                   */
/************************************************************************/
fillMailRoom()  {
int i;

/* mail room -- copy messages in ra.logBuf to room: */
for (i=0;  i<MSGSPERRM;  i++) {
   ra.roomBuf.msg[i].rbmsgLoc  = FALSE /*ERROR*/;
   ra.roomBuf.msg[i].rbmsgNo   = ERROR;
   }
for (i=0;  i<MAILSLOTS;  i++) {
   if (i==MSGSPERRM)   break;  /* for MSGSPRRM < MAILSLOTS */
   ra.roomBuf.msg[i].rbmsgLoc  = ra.logBuf.lbslot[i];
   ra.roomBuf.msg[i].rbmsgNo   = ra.logBuf.lbId[i]  ;
   }
noteRoom();
return TRUE;
}

/************************************************************************/
/* gotoRoom() is the menu fn to travel to a new room     */
/* returns TRUE if room has new msgs, else FALSE         */
/************************************************************************/
char gotoRoom(nam)
char *nam;
{
int  i, foundit, newStuff, roomNo, tmpRm;
char temp[NAMESIZE]; 
unsigned long int nwest;
struct roomBufer rbuf;

foundit = FALSE; /* leaves us in Lobby> if nothing found */
newStuff= FALSE;

if (!strLen(nam)) {  /* just goto, not dot goto */

   /* update log entry for current room:  */
   if (ra.loggedIn && (did_read || ra.thisRoom == MAILROOM) ){ 
      ra.logBuf.lbgen[ra.thisRoom] = ra.roomBuf.rbgen << GENSHIFT;
      did_read=FALSE;
   }
   
   tmpRm = ((ra.thisRoom +1) < MAXROOMS) ? (ra.thisRoom + 1) : 0;
      
/*   for (i   = tmpRm/*0*/; i != ra.thisRoom /*<MAXROOMS*/  &&  !foundit;
        (i = ((i+1) % MAXROOMS)) ) {
*/
   for (i   = tmpRm; i <MAXROOMS  &&  !foundit; i++)
        {
      
      if ((ra.roomTab[i].rtflags & INUSE)  &&
         (ra.roomTab[i].rtgen == (ra.logBuf.lbgen[i]>>GENSHIFT)
          || ((ra.logBuf.BIO_waste[i/8] & bits[i%8]) && 
               ra.roomTab[i].rtflags & BY_INVITE_ONLY))) {
             nwest = ra.logBuf.lbvisit[ra.logBuf.lbgen[i] & CALLMASK]+1;

        if (ra.roomTab[i].rtlastMessage >= nwest){
 /*          if (i != ra.thisRoom && (i != AIDEROOM   ||   ra.aide) &&
              (i != LOGROOM    || (ra.logBuf.lbflags & SYSOP)) )*/   {
                foundit = i;
                newStuff= TRUE;
            }
        }
        if (ra.roomTab[i].rtflags & BY_INVITE_ONLY)
           if(!(ra.logBuf.BIO_waste[i/8] & bits[i%8])) {
             foundit = FALSE;
             newStuff = FALSE;
             }
      }
   }

   getRoom(foundit, &ra.roomBuf);

} 
else {
      sToUpper(nam);

      /* non-empty room name, so now we look for it: */
      /* first by the whole name */
      if (((roomNo = roomExists(nam)) == ERROR)) /*  ||
         (roomNo==AIDEROOM  &&  !(/*ra.onConsole  &&*/  ra.aide)) ||
         (roomNo==LOGROOM   &&  !(/*ra.onConsole  && */ ra.aide)))
         */  --ra.rtry;
      else
         foundit=TRUE;

      if ( foundit && (ra.roomTab[roomNo].rtflags & BY_INVITE_ONLY))
           if(!(ra.logBuf.BIO_waste[roomNo/8] & bits[roomNo%8])) {
             foundit = FALSE;
             --ra.rtry;
             }

   /* then the sacCit way by parts */
   if ((!foundit) && (roomNo != AIDEROOM) && (roomNo != LOGROOM) ) {
   
      for (i=0; i < MAXROOMS && !foundit; i++) {
   
         if (ra.roomTab[i].rtflags & INUSE &&
            ra.roomTab[i].rtgen == (ra.logBuf.lbgen[i]>>GENSHIFT)){
               strcpy(temp, ra.roomTab[i].rtname);
               sToUpper(temp);
      
               if ( strstr(temp, nam) != NULL ) {
                  roomNo = i;
                  foundit = TRUE;
                 ++ra.rtry;
               }
          }
      }
   }

   
   if (foundit)  {
      if(ra.roomTab[roomNo].rtflags & BY_INVITE_ONLY) {
        if (!(ra.logBuf.BIO_waste[roomNo/8] & bits[roomNo%8]) ) foundit = 0;
     }
   }
         
   /* update log entry for current room:   */
   if (foundit) {
      if (ra.loggedIn && (did_read || ra.thisRoom == MAILROOM) ){ 
         ra.logBuf.lbgen[ra.thisRoom] = ra.roomBuf.rbgen << GENSHIFT;
         did_read=FALSE;
      }
         
      getRoom(roomNo, &ra.roomBuf);
      
      /* it may have been unknown... if so, note it:  */
      if ( (ra.logBuf.lbgen[ra.thisRoom] >> GENSHIFT) != ra.roomBuf.rbgen)
         ra.logBuf.lbgen[ra.thisRoom] = 
            ((ra.roomBuf.rbgen << GENSHIFT) + (MAXVISIT -1) );
      }
   }
setUp(FALSE);
dumpRoom();
return  newStuff;   /* was thisRoom */
}

/************************************************************************/
/* init() -- master system initialization          */
/************************************************************************/
init() {
char     getCh(), readSysTab();
char     *msgFile;
unsigned codend(), endExt(), externs(), topOfMem();
FILE *callfl;

ra.grand_poobah = FALSE;
ra.exitToCpm  = FALSE;    /* not time to quit yet!   */
ra.sizeLTentry = sizeOf(ra.logTab[0]);   /* just had to try that feature */
ra.outFlag = OUTOK;    /* not p(ausing)     */
ra.pullMessage = FALSE;      /* not pulling a message   */
ra.pulledMLoc = ERROR;    /* haven't pulled one either  */
ra.pulledMId  = ERROR;
ra.nopace     = FALSE;
/* ra.debug   = FALSE;*/
ra.loggedIn   = FALSE;
ra.thisRoom   = LOBBY;
ra.whichIO = CONSOLE;
ra.loggedIn   = FALSE;
clr_screen = TRUE;
ansi_tut = no_stop = FALSE;
setUp(TRUE);

if(!(is_door || is_mailer)) modemInit();
else initPort();

for_sysop = FALSE;
fix_flag = 0;
init_modem_hourly = 0;
init_modem_done   = 1;
did_read=FALSE;

/* open message files: */
setSpace(ra.msgDisk, ra.msgUser);
openFile("ctdlmsg.sys",        &ra.msgfl );
setSpace(ra.homeDisk, ra.homeUser);
openFile("ctdlroom.sys", &ra.roomfl);
openFile("ctdllog.sys",  &ra.logfl );

if((ra.callfl = open("CALLLOG.SYS",
   O_RDWR | O_CREAT | O_APPEND | O_BINARY,
   S_IREAD | S_IWRITE))  == -1) {
          printf("\n Error opening CALLLOG.SYS\n \0x07");
          setdisk(2);
          exit(123);
         }

close(ra.callfl); /* need to do this on the fly */


pause_override = FALSE;
p_port = 0;
upld=NULL;

/* start of fix for MSDOS not rolling over date at midnight on some 'puters */
getDaTime();
fix_hour = ra.hour;
fix_month = ra.minute;
fix_date = ra.date;
fix_year = ra.year;
fix_flag = (ra.hour ? 1 : 0);
neverLoggedIn = FALSE;

return TRUE;
}

/************************************************************************/
/* initCitadel() does not reformat data files         */
/************************************************************************/
initCitadel() {
ra.whichIO = MODEM;
setUp(FALSE);
return TRUE;
}

/************************************************************************/
/* listRooms() lists known rooms             */
/************************************************************************/
listRooms(doDull)
char doDull;   /* TRUE to list unchanged rooms also   */
{
char str[NAMESIZE+7];
char boringRooms, doBoringRooms, hasUnseenStuff, shownHidden;
int  i, valid_gencheck;

shownHidden = FALSE;
boringRooms = FALSE;
mPrintf("\n New Messages To Read In:");
doCR();
for (doBoringRooms=FALSE;  doBoringRooms<=doDull;  doBoringRooms++) {
   for (i=0;  i<MAXROOMS;  i++) {
      
      valid_gencheck = (ra.roomTab[i].rtflags & PUBLIC) ||
                       (!(ra.roomTab[i].rtflags & BY_INVITE_ONLY)) ||
                       ( ((ra.roomTab[i].rtflags & BY_INVITE_ONLY) && 
                       (ra.logBuf.BIO_waste[i/8] & bits[i%8])));
                       
      if ((ra.roomTab[i].rtflags & INUSE) &&  
         /* list it if public or previously visited: */
         (((ra.roomTab[i].rtflags & PUBLIC) ||  ( ra.loggedIn   &&
         ra.roomTab[i].rtgen == (ra.logBuf.lbgen[i] >> GENSHIFT) &&
         valid_gencheck) || ra.debug)||
         ( ((ra.roomTab[i].rtflags & BY_INVITE_ONLY) && 
         (ra.logBuf.BIO_waste[i/8] & bits[i%8]))))){
            /* do only rooms with unseen messages first pass,  */
            /* only rooms without unseen messages second pass: */
         
         hasUnseenStuff =  (
            ra.roomTab[i].rtlastMessage >=
            (ra.logBuf.lbvisit[ ra.logBuf.lbgen[i] & CALLMASK ]+1)
            );
         
/* kluge to fix new user seeing Mail> after being untwitted even when none
   is present */
   
   if( i == MAILROOM  && hasUnseenStuff)
     if(ra.logBuf.lbId[MAILSLOTS - 1] == ra.logBuf.lbId[0])
         hasUnseenStuff = FALSE;

            
         if (!hasUnseenStuff)   boringRooms  = TRUE;
         
    if ((!doBoringRooms &&  hasUnseenStuff)  ||
       ( doBoringRooms && !hasUnseenStuff)) {
         sprintf(str,"%d-", i);
         strcat(str, ra.roomTab[i].rtname);

         if (ra.roomTab[i].rtflags & BY_INVITE_ONLY)  strcat(str, "*");
         if (ra.roomTab[i].rtflags & CPMDIR)   strcat(str, "]");
         else          strcat(str, ">");

         if ((! (ra.roomTab[i].rtflags & PUBLIC))
              && (!(ra.roomTab[i].rtflags & BY_INVITE_ONLY))) {
            strcat(str, "{");
         shownHidden = TRUE;
         }
       
         if (ra.roomTab[i].rtflags & ANON  )    strcat(str, "-");
         
         sprintf(ra.sectBuf, "%-35s",str);
         
         if(ra.ansi_on) {
           if (ra.roomTab[i].rtflags & BY_INVITE_ONLY) doANSI(ra.ANSI_header);
           else if (ra.roomTab[i].rtflags & CPMDIR) doANSI(ra.ANSI_author);
                else doANSI(ra.ANSI_prompt);
         }
                  
         putWord(ra.sectBuf);
         mAbort();
         
    }
  }
}
   if (boringRooms && !doBoringRooms  && doDull)  {
      mPrintf("\n \n No New Messages (Until You Enter One) In :");
      doCR();
      }
   }
return TRUE;
}

/************************************************************************/
/* openFile() opens one of the .sys files.         */
/************************************************************************/
openFile(filename, fd)
char *filename;
int  *fd;
{
/* open message file */
if ((*fd = open(filename, O_RDWR)) == ERROR) {
   sprintf(format, "Cannot Open %s", filename);
   doStatusLine(S_MSGS, format);
   setdisk(2);
   exit(100);
   }
lseek(*fd, 0L, SEEK_SET);  /*init for random io*/
return TRUE;
} 

/************************************************************************/
/* readSysTab() restores state of system from SYSTEM.TAB    */
/* returns: TRUE on success, else FALSE         */
/*     destroys SYSTEM.TAB after read, to prevent erroneous re-use */
/*     in the event of a crash.              */
/************************************************************************/
char readSysTab() {

int fd;

if((fd = open("ctdlTabl.sys", O_RDWR)) == -1) {
   doStatusLine(S_MSGS, "Configure System \x07");
   return(FALSE);
   }

_read(fd, &ra, sizeof(ra));

close(fd); 

if ((ra.firstExtern != 'F') || (ra.lastExtern != 'L'))
   {
   cprintf("Table is bad, reconfigure system \n\r \x07");
   return(FALSE);
   }


if(rename("ctdlTabl.sys","ctdlTabl.$$$"))
   {
   cprintf("ctdlTabl.$$$ exists, exit and CONFIG\n\r");
   return(FALSE);
   }

return(TRUE);
}

/************************************************************************/
/* roomExists() returns slot# of named room else ERROR      */
/************************************************************************/
int roomExists(room)
char *room;
{
int i;

for (i=0;  i<MAXROOMS;  i++) {
   if (ra.roomTab[i].rtflags & INUSE    &&
      strCmpU(room, ra.roomTab[i].rtname) == SAMESTRING) return(i);
   }

return(ERROR);
}

/************************************************************************/
/* setSpace() moves us to a disk and user#         */
/************************************************************************/
setSpace(disk, user)
char disk, *user;
{
int stat;

setdisk((int) disk);
ra.ourDisk = disk;
if (ra.debug) cprintf("\n\r Changed to %c:%s\n\r", ('A'+disk), user);
if(!(stat = chdir(user)))
  strcpy(ra.ourUser, user);
else{
  setdisk(ra.homeDisk);
  chdir(ra.homeUser);
  }
  
return stat;
}

/************************************************************************/
/* setUp()                    */
/************************************************************************/
setUp(justIn)
char justIn;
{
int g, i, j, ourSlot;

ra.echo    = BOTH;  /* just in case      */
ra.usingWCprotocol  = FALSE; /* also redundant    */

if (!ra.loggedIn)   {
   ra.prevChar    = ' ';
   ra.termWidth   = 79;
   ra.termLF       = LFMASK;
   ra.termUpper   = 0;
   ra.termTab      = TABMASK;
   ra.expert       = EXPERT;
   ra.lasto     = LOMASK;
   ra.aide      = FALSE;
   ra.twit        = TWIT;
   ra.termMore     = MORE;
   scrn           = 0;
   ra.pause_at = 21;
   ra.nopace   = FALSE;
   byeflag = TRUE;
   ra.grand_poobah = FALSE;  
   if (justIn)   {
      memset(&ra.logBuf, 0, sizeof(ra.logBuf) );
      
      /* set up ra.logBuf so everything is new...     */
      for (i=0;  i<MAXVISIT;  i++)  ra.logBuf.lbvisit[i] = ra.oldestLo;

      /* no mail for anonymous folks: */
      ra.roomTab[MAILROOM].rtlastMessage = ra.newestLo;
      for (i=0;  i<MAILSLOTS;  i++)   ra.logBuf.lbId[i]  = 0;

      
      ra.logBuf.lbname[0] =0;

      for (i=0;  i<MAXROOMS;  i++) {
        if (ra.roomTab[i].rtflags & PUBLIC) {
        /* make public rooms known: */
           g       = ra.roomTab[i].rtgen;
           ra.logBuf.lbgen[i]  = (g << GENSHIFT) + (MAXVISIT-1);
        }
        else {
           /* make private rooms unknown: */
           g        = (ra.roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
           ra.logBuf.lbgen[i] = (g << GENSHIFT) + (MAXVISIT-1);
        }
      }
    }
} 
else {
   /* ra.loggedIn: */
   ra.termWidth   = ra.logBuf.lbwidth;
   ra.termLF       = ra.logBuf.lbflags & LFMASK ;
   ra.termUpper   = ra.logBuf.lbflags & UCMASK ;
   ra.expert       = ra.logBuf.lbflags & EXPERT ;
   ra.lasto     = ra.logBuf.lbflags & LOMASK ;
   ra.termTab     = ra.logBuf.lbflags & TABMASK;
   ra.aide      = ra.logBuf.lbflags & AIDE   ;
   ra.twit        = ra.logBuf.lbflags & TWIT   ;
   ra.termMore    = ra.logBuf.lbflags & MORE;
   ra.nopace      = ra.logBuf.lbnulls & NOPACE;
   if (ra.termMore && !(ra.logBuf.pause_at & 0x7f)) ra.logBuf.pause_at = 21;
   ra.pause_at    = ra.logBuf.pause_at;
   scrn = 0;
      
   if (justIn)   {
      
      /* set gen on all unknown rooms  --  INUSE or no: */
      for (i=0;  i<MAXROOMS;  i++) {
        
        if (!(ra.roomTab[i].rtflags & PUBLIC)) {
        /* it is private -- is it unknown? */
      
           if ((ra.logBuf.lbgen[i] >> GENSHIFT)  !=   ra.roomTab[i].rtgen) {
              /* yes -- set  gen = (realgen-1) % MAXGEN */
              j = (ra.roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN;
              ra.logBuf.lbgen[ i ] =  (j << GENSHIFT) + (MAXVISIT-1);
          }
       }
       else
          if ((ra.logBuf.lbgen[i] >> GENSHIFT) !=  ra.roomTab[i].rtgen)  {
             /* newly created public room -- remember to visit it; */
             ra.logBuf.lbgen[i] = (ra.roomTab[i].rtgen << GENSHIFT) +1;
          }
     }
     
     /* special kludge for Mail> room, to signal new mail:   */
     ra.roomTab[MAILROOM].rtlastMessage = ra.logBuf.lbId[MAILSLOTS-1];

     /* slide lbvisit array down and change lbgen entries to match: */
     for (i=(MAXVISIT-2);  i;  i--) {
       ra.logBuf.lbvisit[i] = ra.logBuf.lbvisit[i-1];
     }
     
     ra.logBuf.lbvisit[(MAXVISIT-1)]    = ra.oldestLo;
     
     for (i=0;  i<MAXROOMS;  i++) {
        if ((ra.logBuf.lbgen[i] & CALLMASK)  <  (MAXVISIT-2)) {
           ra.logBuf.lbgen[i]++;
        }
     }

     /* Slide entry to top of log table: */
     ourSlot = ra.logTab[ra.thisSlot].ltlogSlot;
     slideltab(0, ra.thisSlot);

     ra.logTab[0].ltpwhash      = hash(ra.logBuf.lbpw);
     ra.logTab[0].ltnmhash      = hash(ra.logBuf.lbname);
     ra.logTab[0].ltlogSlot     = ourSlot;
     ra.logTab[0].ltnewest      = ra.newestLo;
   } 
}

ra.logBuf.lbvisit[0]   = ra.newestLo;

ra.onConsole  = (ra.whichIO == CONSOLE);

if (ra.thisRoom == MAILROOM) fillMailRoom();

ra.echo = BOTH  ;

return TRUE;
}

/************************************************************************/
/* sToUpper() converts a whole string to uppercase       */
/************************************************************************/
char *sToUpper(s)
     char *s;

{
char *start;
int length, i;

start = s;
length = strlen(s);

for (i=0; i < length; i++)
    s[i] = toupper(s[i]);

return(start);
}

/************************************************************************/
/* systat() prints out current system status       */
/************************************************************************/
systat() {
int i;

printDate();

sprintf(format, "\n  BBS Software: %s ", version);
mPrintf(format);

sprintf(format, "\n  Built: %s\n ", babedate);
mPrintf(format);

sprintf(format," %s @ %s\n ", ra.logBuf.lbname, 
        ra.onConsole ? "CONSOLE" : ra.baud_asc);
        
mPrintf(format);

sprintf(format, " online %d min \n  %d Col %s %sLF %s %s\n ",
   timeOn(),
   ra.termWidth,
   ra.termUpper  ?  "UC "    : "",
   ra.termLF      ?  ""       : "no ",
   ra.lasto    ?  "1 old on new "  : "",
   ra.twit       ?  "HELD"   : ""
   );
mPrintf(format);
sprintf(format, " %lu msgs to %lu\n ",
   ra.newestLo-ra.oldestLo +1,
   ra.newestLo);
mPrintf(format);
sprintf(format, " in %dK, with %d users\n ", ra.maxMSector >> 1, MAXLOGTAB );
mPrintf(format);
sprintf(format, " Number of logins: %d\n ", ra.callerno);
mPrintf(format);

return TRUE;
}


/************************************************************************/
/* unspace() copies a filename, removing blanks       */
/************************************************************************/
unspace(from, to)
char *from, *to;
{
while (*to =  *from++)   if (*to != ' ')   to++;
return TRUE;
}

/************************************************************************/
/* wildCard()                    */
/* Adapted from YAM which borrowed from Richard Greenlaw's expander. */
/************************************************************************/
#define UFNsIZE        13  /* size of "filename.ext" plus a null. */

wildCard(fn, filename)
int (*fn)();
char *filename;   /* may be ambiguous.  No drive or user numbers. */
{
char full_path[MAXPATH+15];
/* int strCmp(); */
struct ffblk fildata;
char *p, *q, *fp;

char *filen;
int i,j,k,m,filecount;

if (filename[1] == ':')
   {
   mPrintf("Bad Filename \n");
   return;
   }
if(setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser)){
  mPrintf("\n Directory Not Found - Please Mail Sysop\n ");
  return;
  }
  
getDfr();

if(!(strchr(filename,'.'))) strcat(filename, ".*");
else if((strchr(filename,'.') + 1) == strchr(filename, 0))
     strcat(filename,"*");
getcwd(full_path, MAXPATH);
strcat(full_path,"\\");
strcat(full_path, filename);

strcpy(&fildata.ff_name, "             ");

if ((i = findfirst(full_path, &fildata, 0)) == -1)
   {
   mPrintf("No such file exists \n");
   setSpace(ra.homeDisk, ra.homeUser);
   return;
   }

ra.FDSectCount = 0;
m = 0;
filecount = 0;
fp = &ra.msgBuf.mbtext /* + 1*/;

while( i != -1 && m < MAXTEXT-100)
   {
   filecount++;
   ra.FDSectCount += (unsigned long) fildata.ff_fsize;
   filen = &fildata.ff_name;

   for(j=0; (j < 8) && (fildata.ff_name[j] != '.'); ) 
      *fp++ = fildata.ff_name[j++];
   k = j;

   while(j++ < 8) *fp++ = ' ';
   *fp++ = '.';
   k++;

   while((fildata.ff_name[k] != '\0') && (j < 12))
      {
      *fp++ = fildata.ff_name[k++];
      j++;
      }

   while(j++ < 12) *fp++ = ' ';
   *fp++ = '\0';

   strcpy(&fildata.ff_name, "             ");

   i = findnext(&fildata);
   }

   qSort(ra.msgBuf.mbtext, filecount, UFNsIZE, strCmp);

   ra.outFlag = OUTOK;

  doCR();

  for (fp = ra.msgBuf.mbtext; filecount-- && ra.outFlag != OUTSKIP; 
       fp += UFNsIZE)  (*fn)(fp);

setSpace(ra.homeDisk, ra.homeUser);

return TRUE;
}

/************************************************************************/
/* writeSysTab() saves state of system in SYSTEM.TAB     */
/* returns: TRUE on success, else ERROR         */
/************************************************************************/
writeSysTab() 

{

int fd;

setSpace(ra.homeDisk, &ra.homeUser);

unlink("ctdltabl.$$$");

if((fd = open("ctdlTabl.sys", O_CREAT | O_EXCL, S_IREAD | S_IWRITE )) == -1) {
   cprintf("ctdlTabl exists on create \n\r \x07");
   cprintf("old one not altered \n\r");
   return(FALSE);
   }

close(fd);

ra.firstExtern = 'F';
ra.lastExtern = 'L';

chmod("ctdlTabl.sys", S_IREAD | S_IWRITE);

if((fd = open("ctdlTabl.sys", O_RDWR)) == -1)
   {
   cprintf("error in ctdlTabl.sys attributes \n\r");
   return(FALSE);
   }
_write(fd, &ra, sizeof(ra));
close(fd);
return(TRUE);
}

