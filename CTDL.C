#define globl

#include "sys\locking.h"
#include "ctdl.h"
/************************************************************************/
/* CTDL.C   Command Processor For Citadel/Babel                         */
/*          history           */
/* 92Aug05 JCL Ported to MSDOS  under Borland C++ 2.0               */
/* 86Apr23 BP,KK,MM networks, print, carr, global, etc         */
/* 83Jul14 BAK ...                     */
/* 83Jun11 BAK  Edited to fix <typo>.hlp problem.           */
/* 83Mar08 CrT Aide-special functions installed & tested...    */
/* 83Feb24 CrT/SB Menus rearranged.             */
/* 82Dec06 CrT 2.00 release.                 */
/* 82Nov05 CrT removed main() from room2.c and split into sub-fn()s  */
/************************************************************************/
/************************************************************************/
/*          Contents          */
/*                         */
/* doAide()    handles Aide-only commands    */
/* doChat()    handles C(hat)    command  */
/* doEnter()      handles E(nter)   command  */
/* doGoto()    handles G(oto)    command  */
/* doHelp()    handles H(elp)    command  */
/* doKnown()      handles K(nown rooms)   command  */
/* doLogin()      handles L(ogin)   command  */
/* doLogout()     handles T(erminate)  command  */
/* doRead()    handles R(ead)    command  */
/* doRegular()    fanout for above commands     */
/* doSysop()      handles sysop-only commands      */
/* getCommand()      prints prompt and gets command char */
/* greeting()     System-entry blurb etc        */
/* main()         has the central menu code     */
/************************************************************************/

char iChar();
char carrDet();
extern void cconfg(int x);

/************************************************************************/
/* doAide() handles the aide-only menu          */
/*     return FALSE to fall invisibly into default error msg   */
/************************************************************************/
char doAide(moreYet, first)
char moreYet;
char first; /* first parameter if TRUE    */
{
char oldName[NAMESIZE], new_name[NAMESIZE];
int  logNo, rm, i, h, good;
struct logBuffer lbuf;
char foundit, ch, iChar(), who[NAMESIZE];

if (!ra.aide)  return FALSE;

if (moreYet)   first = '\0';

mPrintf("ide:  ");

if (first)    oChar(first);

switch (ch = ((char) toUpper(   first ? first : iChar()    ))) {
case 'E':
   mPrintf("dit room");
   strCpy(oldName, ra.roomBuf.rbname);
   if (!renameRoom())   break;
   if (ra.whichIO == MODEM)  {
      sPrintf(
    ra.msgBuf.mbtext,
    "%s, %s > %s",
    ra.logBuf.lbname,
    oldName,
    ra.roomBuf.rbname
    );
    if(!(ra.roomBuf.rbflags & BY_INVITE_ONLY) )
      aideMessage( /* noteDeletedMessage == */ FALSE);
      }
   break;

case 'G':
   if (ra.roomBuf.rbflags & BY_INVITE_ONLY) {
      lseek(ra.logfl, 0L, SEEK_SET);
      mPrintf("uest List:\n ");
      for (i = 0; i < MAXLOGTAB; i++) {
          _read (ra.logfl, &lbuf, SECSPERLOG);   /*get a log entry*/
          if (strlen(lbuf.lbname)  /*  &&
             (hash(lbuf.lbname) == hash(oldName)) */  /*real person? */
             && (lbuf.BIO_waste[ra.thisRoom/8] & bits[ra.thisRoom%8])) {
                sprintf(format, "%s\n ", lbuf.lbname);
                mPrintf(format);
          }
      }    
   }
   break;
case 'I':
   mPrintf("nsert");
   if ((ra.thisRoom <= LOGROOM && ra.thisRoom != LOBBY)
      ||
      ( int ) ra.pulledMLoc == ERROR) {
      mPrintf(" Not here");
      break;
      }
   note2Message(ra.pulledMId, ra.pulledMLoc);
   putRoom(ra.thisRoom, &ra.roomBuf);
   noteRoom();
   if (ra.whichIO == MODEM)  {
      sPrintf(
    ra.msgBuf.mbtext,
    "> %s by %s",
    ra.roomBuf.rbname,
    ra.logBuf.lbname
    )  ;
      aideMessage( /* noteDeletedMessage == */ FALSE);
      }
   break;
case 'K':
   mPrintf("ill room");
   if (ra.thisRoom <= LOGROOM) {
      mPrintf(" Can't Kill a System Room!!");
      break;
      }
   if (!getYesNo("OK"))   break;
   if (ra.whichIO == MODEM)  {
      sPrintf(
    ra.msgBuf.mbtext,
    "%s X %s",
    ra.logBuf.lbname,
    ra.roomBuf.rbname
    );
      aideMessage( /* noteDeletedMessage == */ FALSE);
      }
   if (ra.roomBuf.rbflags & CPMDIR) {
      chdir(ra.info_dir);
      sprintf(format, "FILEINFO.%03d", ra.thisRoom);
      unlink(format);
      setSpace(ra.homeDisk, ra.homeUser);
   }   
   ra.roomBuf.rbflags ^= INUSE;
   putRoom(ra.thisRoom, &ra.roomBuf);
   noteRoom();
   getRoom(LOBBY, &ra.roomBuf);
   break;
case 'N':
   mPrintf("ame Change\n ");
      getString("Who", who, NAMESIZE);
      normalizeString(who);
      if(ra.loggedIn) terminate(FALSE);
      logNo   = findPerson(who, &lbuf);
      if (logNo == ERROR)   {
         mPrintf("Not found");
      }       
      else {
         getString("New User Name: ", new_name, NAMESIZE);
      normalizeString(new_name);
      h  = hash(new_name);
      for (i=0, good=TRUE;   i<MAXLOGTAB && good;   i++) {
  
      if (h == ra.logTab[i].ltnmhash) good = FALSE;
      }

      if (!h || h==hash("Citadel") || h==hash("Sysop") || strlen(new_name) < 3){
         good = FALSE;
      }
      
      /* lie sometimes -- hash collision !=> name collision */
      if (!good){
         sprintf(format,"\n %s inuse\n ", new_name);
         mPrintf(format);
      }
      else {
         strcpy(lbuf.lbname, new_name);
         putLog(lbuf, logNo);
         }
    }
    break;

case 'S':
case 'U':
     if (ra.roomBuf.rbflags & BY_INVITE_ONLY) {
        lseek(ra.logfl, 0L, SEEK_SET);
        if (ch == 'S')mPrintf("end Invitations\n ");
        else mPrintf("ninvite Guests\n ");
        mPrintf("Enter '?' For Userlog\n ");
        oldName[0] = '?';
        while (strlen(oldName) != 0) {
           foundit = 0;
           getString("Name", oldName, NAMESIZE);
           if ( strlen(oldName) > 0) {
              lseek(ra.logfl, 0L, SEEK_SET);
              if(oldName[0] == '?') userLog();
              else{
              for (i = 0; (i < MAXLOGTAB) && !foundit; i++) {
                getLog( &lbuf, i);   /*get a log entry*/
                if (strlen(lbuf.lbname)
                        &&
                   (hash(&lbuf.lbname) == hash(oldName))) {  /*real person? */
                   if(ch == 'S')
                      lbuf.BIO_waste[ra.thisRoom/8] |= bits[ra.thisRoom%8];
                   else  
                     if ((hash(oldName) == hash(ra.sysopname)))
                         mPrintf("\n Can't Uninvite Sysop From Any Room \n ");
                     else { 
                       lbuf.BIO_waste[ra.thisRoom/8] &= ~(bits[ra.thisRoom%8]);
                       }
                       mPrintf("Done\n ");
                       putLog(&lbuf, i);
                       foundit = 1;
                       }
                   }
                 }
                 if (!foundit) mPrintf("Not found!\n ");
              }   
            }
        }
        break;               
case 'T':
    mPrintf("wit Toggle\n");                
      getString("who", who, NAMESIZE);
      normalizeString(who);
      logNo   = findPerson(who, &lbuf);
      if (logNo == ERROR)   {
    mPrintf("Not found");
    break;
    }
      lbuf.lbflags ^= TWIT;
      lbuf.lbvisit[0] = 1; /* won't think twit after next confg */
      sprintf(format, "%s", (lbuf.lbflags & TWIT) ? "is now Twitted" : "isn't twitted now" );
      mPrintf(format);
      if (!getYesNo("OK")) break;
      putLog(&lbuf, logNo);
      break;

case '?':
   doCR();
   tutorial("aide.mnu");
   break;
default:
   mPrintf("?"          );
   break;
}
return TRUE;
}

/************************************************************************/
/* doChat()                   */
/************************************************************************/
void doChat(moreYet, first)
char moreYet;  /* TRUE to accept folliwng parameters  */
char first; /* first paramter if TRUE     */
{
if (moreYet)   first = '\0';

if (first)    oChar(first);

mPrintf("hat\n ");

if ((!ra.loggedIn || ra.noChat) && ra.whichIO !=CONSOLE)   {
   tutorial("nochat.blb");
   return;
   }

if (ra.whichIO == MODEM)  ringSysop();
else       interact() ;
}

/************************************************************************/
/* doEnter() handles E(nter) command            */
/************************************************************************/
void doEnter(moreYet, first)
   char moreYet;  /* TRUE to accept following parameters */
   char first; /* first parameter if TRUE    */
{
#define CONFIGURATION   0
#define MESSAGE   1
#define PASSWORD        2
#define ROOM      3
#define FILE      4
char what;       /* one of above five */
char abort, done, WC, holdMore;
char iChar(), upld_ok, outside[128];
/* char *cmd_msg_args[]={ra.msg_editor, "TEMP.MSG",'\0'}; */
int ok, i;

if (moreYet)   first = '\0';

abort   = FALSE  ;
done = FALSE  ;
WC      = FALSE  ;
what = MESSAGE;

if ((!ra.loggedIn  &&  !ra.unlogEnterOk)) {
   gotoRoom("Mail");
   mPrintf("\bTo Sysop");
   ra.textDownload=TRUE;
   MakeMessage(WC);
   return;
   }

if (ra.thisRoom==LOGROOM) {
   return;
   }

mPrintf("nter ");

if (first)    oChar(first);
if (!ra.expert && moreYet) mPrintf("(Config, Room, Name, Passwd, Msg, ?): ");
   do   {
   ra.outFlag = OUTOK;

   if(!ra.expert) {
     oChar('\b');
     oChar('\b');
     for (i = 0; i < 37; i++) mPrintf(" \b\b");
   }

   switch ((char) toUpper(   first ? first : iChar()   )) {
   case '\r':
   case '\n':
      moreYet = FALSE;
      break;
   case 'E':
      mPrintf("xclusive");
      gotoRoom("Mail");
      givePrompt();
      done = TRUE;
      break;    
  case 'F':
      if (ra.roomBuf.rbflags & CPMDIR && !ra.twit) {
    mPrintf("ile");
    WC = TRUE;
    what  = FILE;
    done  = TRUE;
    break;
    }
    else{
    mPrintf("\b\b\b\b\b\bNot A Directory Room']'");
    abort = TRUE;
    break;
    }
  
   default:
      mPrintf("\b?");
      abort   = TRUE;
      if (ra.expert)  break;
   case '?':
      tutorial("entopt.mnu");
      abort   = TRUE;
      break;
   case 'C':
      mPrintf("onfiguration");
      what = CONFIGURATION;
      done = TRUE;
      break;
   case 'M':
      mPrintf("essage" );
      what = MESSAGE      ;
      done = TRUE;
      break;
   case 'X':
      mPrintf("modem Message" );
      what = MESSAGE      ;
      done = TRUE;
      WC = TRUE;
      if (!ra.aide)
      mPrintf("\n \n [ System Will Reject Any Message Over 10000 Bytes ]\n ");
      break;
   case 'A':
      if (ra.onConsole) strcpy(outside, "TOPED.EXE /L");
      else  if (!ra.ansi_on) {
              mPrintf("\n [ You Must Be In ANSI Mode to Use This Editor ]\n ");
              mPrintf(" [ Press <A>nsi toggle at Room Prompt ]\n ");
              return;
              }
           else   sprintf(outside, "TOPED %d", ra.portno + 1);
           unlink("MSGTMP");
      if ((upld = open("TOPED.EXE", O_RDONLY)) == -1) break;
      close(upld);
      doDorinfo();
      strcpy(pc, "MSGTMP");
      mPrintf("\bANSI <full-screen> Editor\n ");
        ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
        
        deinit_fossil();
        system(outside);
        timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
        initPort();
        if(ok)
          puttext( 1, (int) ra.ti.screenheight,
            (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
   
         ra.msgBuf.mbHeadline[0] = '\0';

        if((upld = open("MSGTMP", O_RDONLY)) != -1) {
          if (ra.use_headline)
             getString(" Message Headline ", ra.msgBuf.mbHeadline, 58);
          makeMessage(TRUE);
          close(upld);
          unlink("MSGTMP");
        }
        return; 
   case 'O':
      if (!ra.onConsole) return;
      unlink("MSGTMP");
      if (strlen(ra.msg_editor)) {
        mPrintf("utside Editor\n ");
        strcpy(outside, ra.msg_editor);
        strcat(outside, "  ");
        strcat(outside, "MSGTMP"); 
        ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

        system(outside);
        timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
        
        if(ok)
          puttext( 1, (int) ra.ti.screenheight,
            (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

        ra.msgBuf.mbHeadline[0] = '\0';

        sleep(2);

        strcpy(pc, "MSGTMP");
        if((upld = open("MSGTMP", O_RDONLY)) != -1) {
          if (ra.use_headline)
             getString("\n Message Headline ", ra.msgBuf.mbHeadline, 58);
          makeMessage(TRUE);
          close(upld);
          unlink("MSGTMP");
        }
      }
      return;  
   case 'P':
      mPrintf("assword");
      what = PASSWORD     ;
      done = TRUE;
      break;
   case 'R':
      mPrintf("oom"  );
      if (ra.twit || (!ra.nonAideRoomOk && !ra.aide))   {
    mPrintf(" Not Until Your Verified ");
    abort = TRUE;
    break;
    }
      what = ROOM          ;
      done = TRUE;
      break;
   }
   first = '\0';
   } 
while (!done && moreYet && !abort);

doCR();
if (WC && ra.twit ) {
   tutorial("nox.blb");
   return;
   }

if (!abort) { 
   ra.textDownload = FALSE;
   switch (what) {
   case CONFIGURATION  :   
      configure() ;  
      break;
   case MESSAGE       : 
    {
    if(ra.roomBuf.rbflags & MSG_READ_ONLY) {
      if (!(ra.loggedIn && ((ra.logBuf.lbflags & AIDE) ||
            (ra.logBuf.lbnulls & SYSOP)))) break;
    }  
    holdMore = ra.termMore;
    ra.termMore = 0;
    
    ra.textDownload = TRUE; 
    if(WC) {
      setSpace(ra.homeDisk, ra.homeUser);
      upld = open(  tmpnam(pc), O_RDWR | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
      if (upld == -1) {
         mPrintf("\n Message Upload Error - Aborted...\n ");
         break;                
      }
      if((upld_ok = xreadFile(upld))) {
        if ((filelength(upld) > 10000) && !ra.aide) {
           mPrintf("\n Message Is Too Long, Aborting...\n");
           close(upld);
           unlink(pc);
           break;
           }
        ra.msgBuf.mbHeadline[0] = '\0';
        if (ra.use_headline)
           getString("Message Headline ", ra.msgBuf.mbHeadline, 58);
        lseek(upld, 0, 0);
        mPrintf("\n Storing Net Message\n ");
      }
      else {
        close(upld);
        unlink(pc);
        break;
        }
    }  
/*    if((WC && upld_ok) || !WC) */
      makeMessage(WC); 
      ra.msgBuf.mbHeadline[0] = '\0';
    
    if(WC){
      close(upld);
      unlink(pc);
      }
      
    ra.termMore = holdMore;
    break;
    }
   case PASSWORD      : 
      newPW()  ;  
      break;
   case ROOM       : 
      makeRoom()  ;  
      break;
   case FILE       : 
    {
    ra.textDownload = FALSE; 
    upLoad(); 
    break;
    }
   }
   }
}

/************************************************************************/
/* doGoto() handles G(oto) command           */
/************************************************************************/
void doGoto(expand, first)
char expand;   /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
char roomName[NAMESIZE];

mPrintf("oto ");

if ( !ra.loggedIn ) { 
   mPrintf("Mail");
   gotoRoom("Mail");
   return;
   }

if (!expand) {
   gotoRoom("");
   return;
   }

if(!ra.expert) mPrintf(" Name of Room (Partial Name OK) ===> ");
getString("", roomName, NAMESIZE);
normalizeString(roomName);

if (roomName[0] == '?')   listRooms(/* doDull== */ TRUE);
else       gotoRoom(roomName);
if (!(ra.onConsole || ra.rtry)) {
   tutorial("rtry.blb");
   terminate(TRUE);
   }
}

/************************************************************************/
/* doHelp() handles H(elp) command           */
/************************************************************************/
void doHelp(expand, first)
   char expand;   /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
char fileName[NAMESIZE];
if(!ra.loggedIn) {
   mPrintf("\b");
   tutorial("newu.mnu");
   return;
   } 
if (!expand) {
   mPrintf("elp");
   tutorial("dohelp.hlp");
   return;
   }
mPrintf("elp on What Subject ? ");
getString("", fileName, NAMESIZE);
normalizeString(fileName);
if (fileName[0] == '?'
   || strLen(fileName) > 8
   || fileName[1] == ':'
   || strLen(fileName) < 1
   || strstr(fileName,".") != NULL
   ) {
   tutorial("helpopt.hlp");
   } 
else {
   /* adding the extention makes things look simpler for    */
      /* the user... and restricts the files which can be read */
      strcat(fileName, ".hlp");
   tutorial(fileName);
   }
}

/************************************************************************/
/* doKnown() handles K(nown rooms) command.        */
/************************************************************************/
void doKnown(expand, first)
char expand;   /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
if(!ra.loggedIn){
   mPrintf("\b?");
   return;
   }
mPrintf("\bKnown Rooms");
listRooms(/* doDull== */ TRUE);
if(!ra.expert) {
   doCharPause(1);
   tutorial("partial.hlp");
   }
}

/************************************************************************/
/* doLogin() handles L(ogin) command            */
/************************************************************************/
void doLogin(moreYet, first)
char moreYet;  /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
char passWord[NAMESIZE];
char username[NAMESIZE];

passWord[0] = '\0';
username[0] = '\0';

mPrintf("ogin ");
   if (ra.loggedIn)   {
     mPrintf("Already Logged In!!");
     return;
   }

mPrintf("\n \n *** New Users Press Return ***\n ");

doCR(); 
if (ra.use_username) {
   mPrintf("Username: ");  
   getString("", username, NAMESIZE);   
   }
if (!moreYet) ra.echo = ASTERISK;
else ra.echo = BOTH;

if (!ra.use_username || strlen(username) ) {
   mPrintf("Password: ");
   getString( "", passWord, NAMESIZE);
   }

normalizeString(passWord);
normalizeString(username);
ra.echo = BOTH;
login(passWord, username);
}

/************************************************************************/
/* doLogout() handles T(erminate) command          */
/************************************************************************/
void doLogout(expand, first)
   char expand;   /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
char iChar();

if (expand){
     first = '\0';
     mPrintf("\bTerminate/Stay ");
   }
   
   switch ((char) toUpper( first ? first : iChar()    )) {

   case 'Q':
   case 'T':
   
     if (!expand)   {
         mPrintf("\bTerminate");
         if (!getYesNo("End Session "))   break;
        }
     terminate( /* hangUp == */ TRUE);
     break;
   
   case 'S':
   
     mPrintf("tay");
     terminate( /* hangUp == */ FALSE);
     break;
   }
}

/************************************************************************/
/* doRead() handles R(ead) command           */
/************************************************************************/
void doRead(moreYet, first)
char moreYet;  /* TRUE to accept following parameters */
char first; /* first parameter if TRUE    */
{
char iChar(), holdMore;
int  fileDir(), transmitFile();
long dlT;
char abort, doDir, done, hostFile, whichMess, revOrder, status, WC;
char fileName[NAMESIZE], xsendfile(), *here_is;
int i;

/* 
       variable cps is characters per second at different baud rates
       assuming a 90% protocol efficiency. Xmodem will run about 80%
       and Y/Zmodem closer to 95%. The values are used to calculate
       the length of time it will take to download a file based on
       file size
    */

   static unsigned long cps[] = {
   27, 108, 216, 432, 864, 1728, 3456};


if (moreYet)   first = '\0';

if (!ra.loggedIn  &&  !ra.unlogReadOk)
   {
   mPrintf("\b?");
   return;
   }
mPrintf("\bread ");

if (first)    oChar(first);
if (!ra.expert && moreYet) 
   mPrintf("(Rvrs, Fwd, Global, Status, Callerlog, ?): ");
holdMore = ra.termMore;

abort   = FALSE;
doDir   = FALSE;
done = FALSE;
hostFile   = FALSE;
revOrder   = FALSE;
status  = FALSE;
WC      = FALSE;
whichMess  = NEWoNLY;
ra.textDownload= TRUE;

   do {
   ra.outFlag = OUTOK;

   if(!ra.expert) {
     oChar('\b');
     oChar('\b');
     for (i = 0; i < 43; i++) mPrintf(" \b\b");
   }

   switch ((char) toUpper(   first ? first : iChar()   )) {
   
   case '\n':
   case '\r':
      moreYet = FALSE;
      break;
   case 'F':
      mPrintf("orward ");
      revOrder   = FALSE;
      whichMess  = OLDaNDnEW;
      done = TRUE;
      break;
   case 'G':
      mPrintf("lobal New messages ");
      whichMess  = GLOBALnEW;
      pause_override = TRUE;
      done = TRUE;
      break;  
   case 'L':
      mPrintf("ast 5 Messages ");
      whichMess = LASTFIVE;
      done = TRUE;
      break;   
   case 'N':
      mPrintf("ew ");
      whichMess  = NEWoNLY;
      done = TRUE;
      break;
   case 'O':
      mPrintf("ld ");
      revOrder   = TRUE;
      whichMess  = OLDoNLY;
      done = TRUE;
      break;
   case 'R':
      mPrintf("everse ");
      revOrder   = TRUE;
      whichMess  = OLDaNDnEW;
      done = TRUE;
      break;
   case 'S':
      mPrintf("tatus\n ");
      status  = TRUE;
      done = TRUE;
      break;
   case 'U':
      mPrintf("serlog");
      abort   = TRUE;
      userLog();
      break;

   case 'W':
      mPrintf("\bWC Protocol ");
      WC      = TRUE;
      break;

   case 'C':
      mPrintf("allerlog");
      abort   = TRUE;
      callerLog();
      break;
   case 'B':
      if (ra.roomBuf.rbflags & CPMDIR && !ra.twit)   {
    mPrintf("\bXmodem");
    WC    = TRUE ;
    done     = TRUE ;
    hostFile = TRUE ;
    ra.textDownload   = FALSE;
    break;
    }
   case 'D':
      if (ra.roomBuf.rbflags & CPMDIR && !ra.twit)   {
    mPrintf("irectory ");
    doDir = TRUE;
    done  = TRUE;
    break;
    }
   case 'T':
      if (ra.roomBuf.rbflags & CPMDIR && !ra.twit)   {
    mPrintf("extFile");
    done     = TRUE;
    hostFile = TRUE;
    ra.textDownload   = TRUE;
    break;
    }

   case '?':
      tutorial("readopt.mnu");
      abort   = TRUE;
      break;


   default:
      mPrintf(" ?");
      abort   = TRUE;
      setUp(FALSE);
      if (ra.expert)   break;
   }
   first = '\0';
   } 
while (!done && moreYet && !abort);

if (abort) {
   ra.termMore = holdMore;
   return;
}   

if (status) {
   systat();
   ra.termMore = holdMore;
   return;
   }
doCR();
if (WC && ra.twit ) {
   tutorial("nox.blb");
   ra.termMore = holdMore;
   return;
   }
if (doDir) {
   ra.FDSectCount = 0;  /* global fileDir() totals sectors in  */

   if (!ra.expert) tutorial("file.blb");
   getString("file mask <press enter for all>", fileName, NAMESIZE);

   if(strchr(fileName, ':') || strchr(fileName, '\\') )
     { mPrintf("\n Invalid Character in Filename!! \n ");
       return;
     }  


   normalizeString(fileName);

   sToUpper(fileName);

   i=-1;

   if (strlen(fileName) > 0) {
      while( *DOS_devices[++i] != '\0'){
         if(!strcmp(DOS_devices[i], fileName) )
           {  mPrintf("\n Invalid Filename \n ");
              return;
           }    
      }
   }   

   if (strLen(fileName))   wildCard(fileDir, fileName);
   else        wildCard(fileDir, "*.*"   );
   doCR();
   sprintf(format, "%luK Free on %c: \n",
      ra.freeSpace,
    ('A'+ ra.roomBuf.rbdisk));
   mPrintf(format);
   ra.termMore = holdMore;
   return;
   }

if (hostFile) {
   
   if (!ra.expert) tutorial("file.blb");
   getString("filename", fileName, NAMESIZE);

   dlT = 0;
   
   if(strchr(fileName, ':') || strchr(fileName, '\\') )
     { mPrintf("\n Invalid Character in Filename!! \n ");
     return;
   }  


   normalizeString(fileName);

sToUpper(fileName);

i=-1;

while( *DOS_devices[++i] != '\0'){
   if(!strcmp(DOS_devices[i], fileName) )
     {  mPrintf("\n Invalid Filename \n ");
        return;
     }    
}

if(( (here_is = strpbrk(fileName, "..")) != NULL) && ra.textDownload) {
   i = -1;
   here_is++;
   
   while( *no_view[++i] != '\0') {
      if(!strcmp(no_view[i], here_is)) {
         mPrintf("\n Cannot View That File Type \n ");
         return;
      }
   }
}   
         
   ra.FDSectCount = 0;
   if(strLen(fileName))  { 
      wildCard(fileDir, fileName); 
      }
   else           {  
      return; /* wildCard(fileDir, "*.*"   ); */
      }
   if (ra.FDSectCount) { 
      doCR();
      if(ra.baud > 0 && ra.baud < RING)
        dlT = 1 + (long)( (ra.FDSectCount * 9) / ra.baud  );
      sprintf(format, "%ld bytes takes approx. %ld min. %2ld sec.",
              ra.FDSectCount, dlT/60, dlT%60);
      mPrintf(format);  
      if ( !ra.aide && ((dlT/60) > ((ra.time_allowed + 15) - timeOn())) ){
    mPrintf("Not enough time left in session");
    return;
    }

      if (WC && (!ra.expert)) tutorial("wcdown.blb");
      
      
      if (ra.textDownload){
         if (!getYesNo("Ready"))  return;
/*         ra.usingWCprotocol = WC;  */
         wildCard(transmitFile, fileName);
         } 
      else{ if(getYesNo("Xmodem-1K")){
              if (!getYesNo("Ready"))  return;
              ra.usingWCprotocol = WC;
              xsendfile(fileName,1024);
              }
            else{
               if (!getYesNo("Ready"))  return;
               ra.usingWCprotocol = WC;
               xsendfile(fileName,128);
            }   
          }        
         
      ra.usingWCprotocol = FALSE;
      sPrintf(
    ra.msgBuf.mbtext, "%s from %s to %s",
    fileName,
    ra.roomBuf.rbname,
    ra.logBuf.lbname);
      logMsg();
      }
   ra.termMore = holdMore;
   return;
   }

if(WC) {
   if (whichMess == GLOBALnEW
      ||
      !getYesNo("Xmodem messages"))  return;
   download(whichMess, revOrder);
   ra.termMore = holdMore;
   did_read = TRUE;
   return;
   }

if (whichMess != GLOBALnEW)   {
   showMessages(whichMess, revOrder);
   did_read = TRUE;
   }
else {
   while (ra.outFlag != OUTSKIP   &&    gotoRoom(""))  {
      givePrompt();
      showMessages(NEWoNLY, revOrder);
      did_read = TRUE;
      }
   pause_override = FALSE;
   }
ra.termMore = holdMore;
}
/************************************************************************/
/* doRegular()                   */
/************************************************************************/
char doRegular(x, c)
   char x, c;
{
char toReturn, ch, *reset;
int a;
void transmitFILEINFO();

toReturn = FALSE;

switch (c) {

case 'C': 
   if (ra.haveCarrier) doChat(  x, '\0');        
   break;
case 'D': 
/*   doRead(  x, 'd' );        */
   if (ra.doors && ra.loggedIn && carrDet() ) {
       mPrintf("\bDoors: \n ");
       doDoors();
   }
   else mPrintf("\bDoors Not Available\n ");    
   break;
case 'E': 
   doEnter( x, 'm' );        
   break;
case 'G': 
   did_read = TRUE;
   doGoto(  x, '\0');        
   break;
case 'H': 
   doHelp(  x, '\0');        
   break;
case 'J':
   if (ra.loggedIn)  doJump();
   else mPrintf("\bJust Press 'L' to Login\n ");
   break;   
case 'K': 
   doKnown( x, '\0');        
   break;
case 'L': 
   if (!ra.loggedIn) doLogin( x, '\0');        
   else doRead( x, 'l');
   break;
case 'N': 
   doRead(  x, 'n' );        
   break;
case 'O':
   doRead(  x, 'o' );        
   break;
case 'Q':
   mPrintf("\bQuack!!\n ");
   break;
case 'R': 
   doRead(  x, '\0');        
   break;
/* case 'S':*/
case 'B':
   did_read = FALSE;
/*   mPrintf("\bSkip Room... G"); */
   mPrintf("\bBypass Room... G");
   doGoto(x, '\0');   
   break;
case 'T': 
   doLogout(x, 'q' );        
   break;
/* case 'U':
   did_read = TRUE;
   mPrintf("\bUpdate Message Pointer... G");
   doGoto(x, '\0');   
   break;
*/   
case 'V': 
   doRead(  x, 't' );        
   break;
case 'Y':
   if (!ra.loggedIn) break;
   mPrintf("\bYour Setup\n ");
   configure();
   break;
case 0:
   if (ra.newCarrier)   {
      if( ! (is_mailer || is_door || no_cit_init)) greeting();
      ra.newCarrier = FALSE;
   }
   break;   /* irrelevant value */

case '?':
   if (!ra.loggedIn) {
      tutorial ("newu.mnu");
      break;
      }
   if(!x) tutorial("mainopt.mnu");
   else tutorial("emainopt.mnu");
   break;

case 'M':
   mPrintf("\bGoto Mail");
   gotoRoom("Mail");
   break;


case 'I':
   mPrintf("ntro");
   tutorial ("intro.msg");
   break;

case 'F':
   if (!(ra.roomBuf.rbflags & CPMDIR) || ra.twit ) break;
   ch = 'F';
   setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);

   while( (ch == 'F' || ch == 'R') && (carrDet() || ra.onConsole) ) {
     if (ra.roomBuf.rbflags & NO_UPLOADS) 
        mPrintf("\n \n [ NO UPLOADS ALLOWED IN THIS ROOM ] ");
     mPrintf("\n \n R)ead Directory\n ");   
     mPrintf("F)ile Info\n ");
     if (!ra.onConsole) {
        mPrintf("D)ownload\n ");
        if(!(ra.roomBuf.rbflags & NO_UPLOADS)) mPrintf("U)pload\n ");
        }
     mPrintf("Selection: ");
     ch = '\0';
     ch = (char) toupper(iChar() );
     switch (ch) {
       case 'R' : doRead(  x, 'd' );       
                  break;
       case 'F' : transmitFILEINFO(); 
                  break;
         case 'U' : 
         case 'D' : if (!ra.onConsole) {
                       doXfer(ch); 
                       }
                    setSpace(ra.homeDisk, ra.homeUser);
                    break; 
         default : mPrintf("\b");    
         }
    }
   setSpace(ra.homeDisk, ra.homeUser);
   break;      
/* #endif*/

   case 'A': 
      if(!x) {
        mPrintf("\bANSI Toggle: ANSI Now ");
        ra.ansi_on ^= TRUE;
        mPrintf( ra.ansi_on ? "ON\n " : "OFF\n ");
        if (ra.ansi_on) {
           doANSI(ra.ANSI_bkgd);
           doANSI(ra.ANSI_text);
       }    
       else {
           reset = ANSI_reset;
           while ( *reset != '\0') outMod(*reset++);
           doCR();
           }
        toReturn=TRUE;
        break;
      }  
       else if (!doAide(x, '?'))  toReturn=TRUE;    
   break;
   /*    case 'Z':   won't work without a skip flag
       if (ra.thisRoom <= LOGROOM) break;
       mPrintf("ap Room");
       if (!getYesNo("OK")) break;
            ra.logBuf.lbgen[ra.thisRoom]=
              ((ra.roomBuf.rbgen + MAXGEN/2) % MAXGEN)<<GENSHIFT;
            storeLog();
       getRoom(0,&ra.roomBuf);
       setUp(FALSE);
       dumpRoom();
       break;
*/    default:
   toReturn=TRUE;
   break;
}

return  toReturn;
}

/************************************************************************/
/* doSysop() handles the sysop-only menu           */
/*     return FALSE to fall invisibly into default error msg   */
/************************************************************************/
char doSysop(c, first)
char c;
char first; /* first parameter if TRUE    */
{
char    who[NAMESIZE];
/*    char          bios(); 
*/   struct logBuffer   lBuf;
int     logNo, ltabSlot, ok;
char *null_args[]={ra.term_prg, '\0'};

if (!ra.onConsole) 
   if (ra.loggedIn && !(ra.logBuf.lbnulls & SYSOP)) return TRUE;
while (TRUE)   {

   mPrintf("\n Sysop: ");

   switch ((char) toUpper(   first ? first : iChar()    )) {
   case 'A':
      mPrintf("bort");
      return FALSE;
   case 'B':
      if(ra.haveCarrier || !ra.onConsole) break;
      if(strlen(ra.term_prg)) {
        deinit_fossil();
        mPrintf("\bBBS Dialer\n ");
        ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, 
           ra.msgBuf.mbtext);

/*        system("cls");*/
        clrscr();

        system(ra.term_prg);
        
/*        system("cls");*/
        clrscr();
        setSpace(ra.homeDisk, ra.homeUser);
        setStatusLine();
        if(ok)
          puttext( 1, (int) ra.ti.screenheight,
            (int) ra.ti.screenwidth, (int) ra.ti.screenheight, 
            ra.msgBuf.mbtext);
        
        timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
        initPort();
        setSpace(ra.homeDisk, ra.homeUser);
      }  
      break;
   case 'C':
      sprintf(format, "hat=%s", (!(ra.noChat = !ra.noChat)) ? "On" : "Off");
      mPrintf(format);
      format[0] = (ra.noChat ? ' ' : 'C');
      doStatusLine(T_CHAT, format);
      break;
   case 'D':         /* to see hidden rooms and modem stuff */
      if (!ra.onConsole)  break;
      sprintf(format, "ebug=%s", (ra.debug = !ra.debug) ? "On" : "Off");
      mPrintf(format);
      break;
   case 'N':  /* toggle nasty twit bit usage */
      if(!ra.onConsole) break;
      sprintf(format,"asty Twit Bit Usage=%s",
             (ra.twitbit = !ra.twitbit) ? "On" : "Off");
      mPrintf(format);
      break;
   case 'U':
      if (!ra.aide) break;
      umap();
      break;   
   case 'H':
      if (!ra.onConsole) break;
      mPrintf("angup");
/*      outMod('A');
      outMod('T');
      outMod('H');
      outMod('\r'); */
      doOnhook();
      break;
   case 'K':
      mPrintf("ill ");
      getString("who", who, NAMESIZE);
      normalizeString(who);
      if(ra.loggedIn) terminate(FALSE);
      logNo   = findPerson(who, &lBuf);
      if (logNo == ERROR)   {
    mPrintf("Not found");
    }       
      else {
    ltabSlot = PWSlot(lBuf.lbpw);
    lBuf.lbname[0] = lBuf.lbpw[0] = '\0';
    lBuf.lbvisit[0] = ERROR; /* fix confg loginit() error */
    putLog(&lBuf, logNo);
    ra.logTab[ltabSlot].ltnewest = ERROR;
    ra.logTab[ltabSlot].ltpwhash = ra.logTab[ltabSlot].ltnmhash = 0;
    sprintf(format, "-%s", who);
    mPrintf(format);
    }
      break;

   case 'O':
      if (!ra.onConsole) break;
      mPrintf("ffhook");
/*      outMod('A');
      outMod('T');
      outMod('H');
      outMod('1');
      outMod('\r');*/
      doOffhook();
      break;
   case 'P':
      normalizeString(ra.sysopname);
      normalizeString(ra.logBuf.lbname);
      if (!(ra.onConsole || 
         ra.grand_poobah)) break;
      mPrintf("\bAide Privileges toggle to");
      getString("who", who, NAMESIZE);
      normalizeString(who);
      logNo   = findPerson(who, &lBuf);
      if (logNo == ERROR)   {
    mPrintf("Not found");
    break;
    }
      lBuf.lbflags ^= AIDE;
      sprintf(format, "%s", (lBuf.lbflags & AIDE) ? "is now aide" : "isn't aide now" );
      mPrintf(format);
      if (!getYesNo("OK"))   break;
      putLog(&lBuf, logNo);
      break;
 case 'R':
      normalizeString(ra.sysopname);
      normalizeString(ra.logBuf.lbname);
      if (!(ra.onConsole ||
         ra.grand_poobah)) break;
      mPrintf("emote Sysop toggle for ");
      getString("who", who, NAMESIZE);
      normalizeString(who);
      logNo   = findPerson(who, &lBuf);
      if (logNo == ERROR)   {
    mPrintf("Not found");
    break;
    }
      lBuf.lbnulls ^= SYSOP;
      if (lBuf.lbnulls & SYSOP) lBuf.lbflags | AIDE;
      sprintf(format, "%s",
         (lBuf.lbnulls & SYSOP) ?
          "is now Remote Sysop" : "isn't Remote Sysop now \n Still is an AIDE");
      mPrintf(format);
      if (!getYesNo("OK"))   break;
      putLog(&lBuf, logNo);
      break;

 case 'S':
      if(!ra.onConsole) break;
      ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

      deinit_fossil();
      
      mPrintf("hell Out to DOS.\n \n Type Exit to Return to BBS\n ");
      system("cls");
      system(""); 
      system("cls");
      setSpace(ra.homeDisk, ra.homeUser);
      timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   

      initPort();
 
      if(ok)
         puttext( 1, (int) ra.ti.screenheight,
                 (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

      setSpace(ra.homeDisk, ra.homeUser);
      break;
 case 'T':
      mPrintf("wit Toggle");
      getString("who", who, NAMESIZE);
      normalizeString(who);
      logNo   = findPerson(who, &lBuf);
      if (logNo == ERROR)   {
    mPrintf("Not found");
    break;
    }
      lBuf.lbflags ^= TWIT;
      lBuf.lbvisit[0] = 1; /* won't think twit after next confg */
      sprintf(format, "%s", (lBuf.lbflags & TWIT) ? "is now Twitted" : "isn't twitted now" );
      mPrintf(format);
      if (!getYesNo("OK")) break;
      putLog(&lBuf, logNo);
      break;

   case 'X':
      if (!ra.onConsole) break;
      mPrintf("\beXit to MSDOS");
      if(!getYesNo("OK"))break;
      if (!ra.onConsole)   break;
      ra.exitToCpm = TRUE;
      return FALSE;

   case 'Y':
      if(!ra.onConsole) {
        mPrintf("\n Must Be On Console to Reconfigure System \n ");
        break;
        }
      mPrintf("\bYour System Configuration|n ");
      cconfg(2);
      break;  
   case '?':
  
   default:
      tutorial("sysop.mnu");
      break;
   }
   }
}

/************************************************************************/
/* getCommand() prints menu prompt and gets command char    */
/* Returns: char via parameter and expand flag as value  -- */
/*     i.e., TRUE if parameters follow else FALSE.    */
/************************************************************************/
char getCommand(c)
char *c;
{
char BBSCharReady(), iChar();
char expand, i;

ra.outFlag = OUTOK;

if (*c != '\0') givePrompt();
while (BBSCharReady())   iChar();        /* eat type-ahead       */

while ((*c = (char) toUpper(iChar()))=='P') mPrintf("\b \b");
expand  = (
   *c == ' '
   ||
   *c == '.'
   ||
   *c == '/'
   );
if (expand) *c = (char) toUpper(iChar());

if (ra.justLostCarrier) {
   ra.justLostCarrier = FALSE;
   doStatusLine(S_MSGS, "Waiting...");
   ra.loggedIn=FALSE;
   }

return expand;
}

/************************************************************************/
/* greeting() gives system-entry blurb etc         */
/************************************************************************/
greeting() {
char c;
/* int a;

a=0;  */
c=15;
ra.loggedIn=FALSE;
setUp(TRUE);
pause(100);

if(!ra.hitEnter){
  if(!(is_door || is_mailer)) sleep(2);
  purge_input_buffer();
  }
else {
while (((receive(2) & 0x7F) != 13) && (--c) && (carrDet() ) ) 
/*   if ((a = receive(2) & 0x7F) != 13) */
      {
      mPrintf("Hit ENTER\n ");
      if (c<10) oChar(BELL);
      }
}

ra.ansi_on=FALSE;
doCR();
doCR();

/* if(ra.haveCarrier || ra.onConsole)
  ra.ansi_on = getYesNo("Use Colors <Must Have ANSI Terminal> ");
if (ra.ansi_on) {
   doANSI(ra.ANSI_bkgd);
   doANSI(ra.ANSI_text);
}    
*/
doCR();
doCR();

sprintf(format, 
        "\n  Welcome to %s \n  Running %s \n ", ra.nodeTitle, version);
mPrintf(format);
mPrintf(" (C) Copyright John Luce 1991, 1992, 1993, 1994 (Freeware)\n ");

outMod('\r');
doCR();
doCR();
if( carrDet() ) {
   ra.termMore = 0;
   tutorial ("prebul.msg");
   ra.termMore = MORE;
   }   

/* if(( ! (is_mailer || is_door)) && carrDet() ) */ {
  gotoRoom("Lobby");
  givePrompt();
  }
}

/************************************************************************/
/* main() contains the central menu code           */
/************************************************************************/
void main(int argc, char *argv[]) {
char c = '\0', x;
char getCommand(), init(), readSysTab(), tester[128];
int i, j, num, lck;
struct logBuffer r;
char baud_line[80];

/* if (FALSE) putChar();*/  /* pick up our own in modem.c */
/* if (FALSE)  getNumber();*/  /* dummy to force load */

/* check to see if CCit is already running */
if ((lck = open("CCIT.LCK", O_RDONLY))!= -1) {
   printf("Cannot Run 2 Copies of CopperCit, type EXIT to return to 1st copy\n");
   exit(0);
   }
else {
   printf("Setting CCit Lock..." );
   i = open("CCIT.LCK", O_CREAT | O_EXCL , S_IREAD | S_IWRITE);
 /*  i = lock(lck, 0, 32767); */
   printf("Error: %d, Value: %d\n", i, errno);
   if(i == -1) {
      printf("Cannot Run 2 Copies of CopperCit, type EXIT to return to 1st copy\n");
      exit(0);
      }
   }
close(i);      
/* don't put any code above here... readSysTab() will defeat it.  */
if (!readSysTab()) {
   cconfg((int) (( argc > 1) ? 1 : 0));          
   if (!readSysTab() ) {
      cprintf("\n\rReconfigure Failed\n\r");
      exit(222);
      }
}
directvideo = no_cit_init = is_door = is_mailer = 0;
if (chdir(ra.bat_dir) == -1) mkdir(ra.bat_dir);
if (chdir(ra.info_dir) == -1) mkdir(ra.info_dir);
setSpace(ra.homeDisk, ra.homeUser);

doBuildProtMenu();
if(argc > 1)
{ for ( i = 1; i <= (argc - 1); i++) {
  strcpy(tester, argv[i]);
  switch ((char) toupper(tester[0])) {
  
     case 'M' :  is_mailer = TRUE; 
                 break;
     case 'D' :  is_door = TRUE;
                 break;
     case 'N' :  no_cit_init = TRUE;
                 break;

     }
  }
}


gettextinfo(&ra.ti);
setStatusLine();

timeout = time(NULL) +  ra.timeout;   

if ( !no_cit_init) {
   init();
   initCitadel();

}
else initPort();

if(!(is_mailer || is_door || no_cit_init)) doOnhook();


if ( is_door || is_mailer ) {
   ra.haveCarrier = TRUE;
   ra.newCarrier  = TRUE;
   ra.whichIO     = MODEM;
   strcpy(baud_line, "CONNECT ");
   if (!no_cit_init) {
      strcat(baud_line, &tester[1]);   
      get_baud_rate(baud_line);
   }   
}

    
setSpace(ra.homeDisk, ra.homeUser);


if(!no_cit_init) {
   ra.callerno = 0;
   if((callfl = fopen("CALLNO.SYS", "r+t"))  == NULL) {
             if((callfl = fopen("CALLNO.SYS","w+t")) == NULL) {
               printf("\n Error Opening CALLNO.SYS \n \0x07");
               setdisk(2);
               exit(124);
             }
             else {
               fprintf(callfl, "%ld", ra.callerno);
               fclose(callfl);
             }
   }
   else {
       fscanf(callfl,"%ld", &ra.callerno);
       fclose(callfl); /* need to do this on the fly */
   }
}
getRoom(0, &ra.roomBuf);  /* load Lobby> */


ra.weAre   = CITADEL;

if (!no_cit_init) greeting();
else givePrompt();

ra.lhour = ra.hour;
ra.lminute = ra.minute;
/* no_cit_init = FALSE; */

if((num = findPerson(ra.sysopname, &r)) != ERROR) {
  r.lbnulls |=  SYSOP;
  r.lbflags |=  AIDE;
  r.BIO_waste[0] |= bits[2];  /* AIDE ROOM */
  r.BIO_waste[0] |= bits[3];  /* LOG ROOM */
  putLog(&r, num);
}

  
while (!ra.exitToCpm) {
   x  = getCommand(&c);

   ra.outFlag = OUTOK;
   StatusLineDate();
   if ((c==CNTRLt)  ?  doSysop(0, '\0')  :  doRegular(x, c))  {
      mPrintf("  '?' for menu"   );
      
      if ( ra.haveCarrier || ra.onConsole ) {
         if ( time(NULL) > timeout) terminate(TRUE);
         }
      }
   else 
      timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
   }
if (ra.loggedIn) terminate( /* hangUp == */ FALSE);
if(!is_door && !is_mailer) {
  if (getYesNo("Hang Up Phone ") ) doOnhook();
  }
else hangUp();  
writeSysTab(); 
unlink("CCIT.LCK");
system("cls");
}


