/************************************************************************/
/* bds1.46  9/89        confg.c        */
/* configuration program for Citadel bulletin board system. */
/************************************************************************/

/* #define globl */

#include "ctdl.h"
#include "isam.h"
/* #include "window.h"*/
#include "stdio.h"
/* #include "field.h"*/

/************************************************************************/
/*          History           */
/* 86April BP   Modified                  */
/* 82Nov20 CrT Created.                */
/************************************************************************/

/************************************************************************/
/*          Contents          */
/*                         */
/* cinit()         system startup initialization    */
/* main()                        */
/* logInit()      builds the RAM index to ctdllog.sys */
/* msgInit()      sets up ra.catChar, catSect etc.    */
/* sortLog()      sort userlog by time since last call   */
/* wrapup()    finishes and writes ctdlTabl.sys */
/* zapLogFile()      erases & re-initializes ctdllog.sys */
/* zapMsgFile()      initialize ctdlmsg.sys        */
/* zapRoomFile()     erase & re-initialize ctdlroom.sys  */
/************************************************************************/


/************************************************************************/
/* cinit() -- master system initialization          */
/************************************************************************/
cinit(argc)
int argc;
{
char     getCh();
/*    unsigned codend(), endExt(), externs(), topOfMem();*/
char     c, *msgFile;

ra.echo    = BOTH;  /* ra.echo input to console too  */
ra.usingWCprotocol  = FALSE;

ra.exitToCpm  = FALSE;    /* not time to quit yet!   */
ra.sizeLTentry = sizeof(ra.logTab[0]);   /* just had to try that feature */
ra.outFlag = OUTOK;    /* not p(ausing)     */

ra.pullMessage = FALSE;      /* not pulling a message   */
ra.pulledMLoc = ERROR;    /* haven't pulled one either  */
ra.pulledMId  = ERROR;
ra.reply   = FALSE;    /* no mail yet       */
ra.debug   = FALSE;
ra.loggedIn   = FALSE;
ra.haveCarrier = FALSE;
ra.lasto   = FALSE;
ra.termWidth  = 79;
ra.termUpper  = FALSE;
ra.termTab = FALSE;

ra.noChat  = TRUE;

/* ra.shave-and-a-haircut/two bits pause pattern for ringing sysop: */
ra.shave[0]   = 40;
ra.shave[1]   = 20;
ra.shave[2]   = 20;
ra.shave[3]   = 40;
ra.shave[4]   = 80;
ra.shave[5]   = 40;
ra.shave[6]   =250;

/* initialize input character-translation table:  */
for (c=0;     c<'\40';  c++) 
   {
   ra.filter[c] = 0;    /* control chars -> nul*/
   }
for (c='\40';  c<=127;    c++)
   {
   ra.filter[c] = c;       /* pass printing chars     */
   }
ra.filter[SPECIAL]  = SPECIAL;
ra.filter[CNTRLp]   = 0;
ra.filter[12    ]   = 0;
ra.filter[26    ]   = 0;
ra.filter[CNTRLt]   = CNTRLt;
ra.filter[CNTRLl]   = CNTRLl;  
ra.filter[DEL     ] = BACKSPACE;
ra.filter[BACKSPACE]   = BACKSPACE;
ra.filter['|'      ]   = '!'      ;  /* to prevent forgery, see ra.date */
ra.filter[CNTRLC   ]   = 'S'      ;
ra.filter[CNTRLI   ]   = CNTRLI   ;   
ra.filter[XOFF     ]   = 'P'    ;
ra.filter['\r'     ]   = NEWLINE  ;
/* ra.filter[CNTRLO   ]   = 'N'    ; */
/*    ra.ra.filter[126      ]   = '-'     ; #* haz1500 protection from tilde *#
*/
ra.weAre = CONFIGURE;
setSpace(ra.msgDisk, ra.msgUser);
if ((ra.msgfl = open("ctdlmsg.sys", O_RDWR )) == ERROR) 
   {
   cprintf("\n\r Creating msg. file \n\r");
   if ((ra.msgfl = creat("ctdlmsg.sys", (S_IREAD | S_IWRITE))) == ERROR)
      {
      cprintf("\n\r Oops! No can do! ");
      exit(1);
      }
   zapMsgFile();   
   }
lseek(ra.msgfl, 0L, SEEK_SET); /*init random io*/

setSpace(ra.homeDisk, ra.homeUser);
/* open room file */
if ((ra.roomfl = open("ctdlroom.sys", O_RDWR )) == ERROR)
   {
   cprintf("\n\r Creating ctdlroom");
   if ((ra.roomfl = creat("ctdlroom.sys", S_IREAD | S_IWRITE)) == ERROR)
      {
      cprintf("\n\r Can't do that! ");
      exit(1);
      }
   zapRoomFile();   
   }

lseek(ra.roomfl, 0L, SEEK_SET); /*init random io*/
getRoom(LOGROOM, &ra.roomBuf);
ra.roomBuf.rbflags |= BY_INVITE_ONLY;
putRoom(LOGROOM, &ra.roomBuf);
getRoom(AIDEROOM, &ra.roomBuf);
ra.roomBuf.rbflags |= BY_INVITE_ONLY;
putRoom(AIDEROOM, &ra.roomBuf);


/* open userlog file */
if ((ra.logfl = open("ctdllog.sys", O_RDWR )) == ERROR) 
   {
   cprintf("\n\r Creating ctdlog\n\r ");
   if ((ra.logfl = creat("ctdllog.sys", S_IREAD | S_IWRITE)) == ERROR)
      {
      cprintf("\n\r Sorry, not my day, no can do! ");
      exit(1);
      }
   zapLogFile();   
   }

lseek(ra.logfl, 0L, SEEK_SET); /*init random io*/

/*if(argc == 1)
   {
   cprintf("\n\r Erase log, message and/or room files?");
   cprintf("\n\r Or is this your first configure?");
   if ( (char) toUpper(getCh()) == 'Y') {

      #* each of these has an additional go/no-go interrogation: *#
      zapMsgFile();
      zapRoomFile();
      zapLogFile();
      }
   }
*/   
return TRUE;
}

/************************************************************************/
/* main() for confg.c                  */
/************************************************************************/
void cconfg(int argc)
/* argc value: 0 = no ctdltabl.sys  1 = no ctdltabl.sys and isdoor or mailer
               2 = manual request to redo   */
{
extern int cfg_interact(void);
char *fBuf;
char *line;
char *cmd, *var, *string;
/*  char fBuf[BUFSIZ];           */
/*  char line[128];           */
/*  char cmd[128], var[128], string[128]; */
int  arg2, args, ctr, dowhat, ok;
char arg[60], *temp;
FILE *fp;
char cnfgfile[40] = "A:";
unsigned long int kb;     
/*  cprintf("Args %d\n", argc); */   
/* icky-tricky to conserve RAM: */
fBuf = (char *) &ra.msgBuf;
line = fBuf + BUFSZE;
cmd  = line + 128;
var  = cmd  + 128;
/*    string  = var  + 128; */
cnfgfile[0] +=  getdisk();
strcat(cnfgfile,"ccitcnfg.sys");

if ((fp = fopen(cnfgfile, "r")) == NULL  ||  argc > 1) {
        ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
        dowhat = cfg_interact();
        if(ok)
          puttext( 1, (int) ra.ti.screenheight,
            (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
   }
   
if ((fp = fopen(cnfgfile, "r")) == NULL ) exit(186);
if (!dowhat) return;
   
ctr = -1;
   
while (fgets(fBuf, BUFSZE /*line*/, fp/*fBuf*/) != NULL) {
   temp = strchr(fBuf, '\n');
   if (temp != NULL) *temp = ' ';
   switch(++ctr) {
    case 32:
         ra.AT = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
         break;      
    case 33:
         ra.time_allowed = atoi(fBuf);
         break;
    case 34:
         ra.use_headline = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
         break;
    case 35:
         ra.use_username = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
         break;
         
    case 18:
       ra.hitEnter = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;   
    case 19:
       ra.doors = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;
    case 15:
       ra.timeout = atol(fBuf);
       break;
    case 7:
       kb = atoi(fBuf);
       kb *= 1024;
       ra.maxMSector = kb/SECTSIZE;
       if(ra.maxMSector > 0x7FFF){
          cprintf("CTDLMSG.SYS can be no bigger than 16 meg\n\r");
          cprintf("Who are you trying to kid anyway? You wanna\n\r");
          cprintf("hold a year's worth of messages??? <grin>\n\r");
          exit(111);
          }
       break;   
    case 9:
       normalizeString(fBuf);
       ra.msgDisk = toupper(fBuf[0]) - 'A';
       strcpy(ra.msgUser, &fBuf[2]);
       break;
    case 16:
       strcpy(ra.msg_editor, fBuf);
       break;   
    case 17:
       strcpy(ra.term_prg, fBuf);
       break;
    case 8:
       normalizeString(fBuf);  
       ra.homeDisk   = toupper(fBuf[0]) - 'A';
       strcpy(ra.homeUser, &fBuf[2]);
       break;
    case 10:
       ra.unlogLoginOk= (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;   
    case 11:
       ra.twitbit = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;
    case 13: 
       ra.unlogEnterOk= (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;
    case 12:
       ra.unlogReadOk = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;
    case 14:
       ra.nonAideRoomOk=(toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;
    case 4:
       ra.portno = atoi(fBuf) - 1;
       break;
    case 5:
       ra.lockedport = (toupper(fBuf[0]) == 'Y' ? 1 : 0);
       break;   
    case 6:
/*       arg2 = atoi(fBuf);       
       ra.hibaud = 0 + (arg2 == 1200 ? 1 : 0) + (arg2 == 2400 ? 2 : 0) +
                   (arg2 == 4800 ? 3 : 0) + (arg2 == 9600 ? 4 : 0) +
                   (arg2 == 19200 ? 5 : 0) + (arg2 == 38400 ? 6 : 0);*/
       ra.hibaud = atol(fBuf);            
       break;            
    case 24:
       ra.ANSI_text = atoi(fBuf) + 30;
       break;
    case 22:
       ra.ANSI_header = atoi(fBuf) + 30;
       break;
    case 23:
       ra.ANSI_prompt = atoi(fBuf) + 30;
       break;
    case 21:
       ra.ANSI_author = atoi(fBuf) + 30;
       break;
    case 20:
       ra.ANSI_bkgd = atoi(fBuf) + 40;
       break;
    case 0:  
       temp = (char *) &ra.nodeTitle;
       strcpy(temp, fBuf);
       break;       
    case 1:
       temp = (char *) &ra.nodeName;
       strcpy(temp, fBuf);
       break;
    case 2:
       temp = (char *) &ra.nodeId;
       strcpy(temp, fBuf);
       break;
    case 25:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.dial_out, fBuf);
       break;   
    case 26:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       temp = (char *) &ra.sModStr;
       strcpy(temp, fBuf);
       break;
    case 27:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.init2, fBuf);
       break;
    case 28:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.on_hook, fBuf);
       break;
    case 29:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.attn, fBuf);
       break;
    case 30:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.off_hook, fBuf);
       break;

    case 31:
       normalizeString(fBuf);
       while((temp=strchr(fBuf, '|')) != NULL) *temp = '\r';
       strcpy(&ra.answer_phone, fBuf);
       break;
                    
    case 3:
       temp = (char *) &ra.sysopname;
       strcpy(temp, fBuf);
       normalizeString(temp);
       break;
   }
}   
if (argc == 2) {
   cprintf("\n\r Reconfiguration Complete \n\r");
   return;
   }
if (argc > 0) mPrintf("\n ==> Please wait... system is reconfiguring.... \n ");
setdisk(ra.homeDisk);
arg2 = mkdir(ra.homeUser);  
sprintf(ra.main_dir,"%c:%s",ra.homeDisk + 'A', ra.homeUser);
strcpy(ra.ansi_dir, ra.main_dir);
strcat(ra.ansi_dir, "\\ANSIHELP");
strcpy(ra.text_dir, ra.main_dir);
strcat(ra.text_dir, "\\TEXTHELP");
strcpy(ra.bat_dir, ra.main_dir);
strcat(ra.bat_dir, "\\BAT");
strcpy(ra.info_dir, ra.main_dir);
strcat(ra.info_dir, "\\FILEINFO");

setdisk(ra.msgDisk);
arg2 = mkdir(ra.msgUser);
setSpace(ra.homeDisk, ra.homeUser);
arg2 = mkdir(ra.ansi_dir);
arg2 = mkdir(ra.text_dir);
arg2 = mkdir(ra.bat_dir);
arg2 = mkdir(ra.info_dir);

doBuildProtMenu();
/* doBuildResults(); */

cinit(argc);
wrapup(argc);

}
/************************************************************************
 doBuildProtMenu  builds the upload and download menus and batch files
                  for file transfers
*************************************************************************/
   
doBuildProtMenu()

{
 FILE *door, *up, *menu, *which_one, *protocol_sys, *xfer, *down;
 char menu_item[72], protocol_name[68], read_in[255], ch, xfername[14];
 char always_x[] = " X) Xmodem (Standard or 1K)\n";

 setSpace( ra.homeDisk, ra.homeUser);
 
 chdir(ra.text_dir);
  
 if(((up = fopen("UPMENU.SYS", "wt")) == NULL) ||
     (( down = fopen("DOWNMENU.SYS", "wt")) == NULL) ||
     (( door = fopen("DOORMENU.SYS", "wt")) == NULL) ){
        doStatusLine(S_MSGS, "Cannot Create Menu Files");
        setSpace( ra.homeDisk, ra.homeUser);
        exit(222);
 }

 fputs(always_x, up);
 fputs(always_x, down);

 chdir(ra.main_dir);
 
 if((protocol_sys = fopen("PROTOCOL.SYS", "rt")) == NULL) {
        fclose(up);
        fclose(down);
        return;
 }
 
 while(fgets(read_in, 254, protocol_sys) != NULL) {
   ch = toupper(read_in[0]);
   if (ch == 'U' || ch == 'D' ) {
      which_one = ( ch == 'U' ? up : down);
      xfername[0] = ch;
      fgets(read_in, 254, protocol_sys);
      ch = toupper(read_in[0]);
      xfername[1] = ch;
      xfername[2] = '\0';
      strcat(xfername, ".bat");
      fgets(read_in, 254, protocol_sys);
      strcpy(protocol_name, read_in);
      sprintf(menu_item, " %c) %s", ch, protocol_name);
      fputs(menu_item, which_one);
      fgets(read_in,254,protocol_sys);
      
      /* clean up macros */
      doMacros(read_in);
      chdir(ra.bat_dir);
      xfer = fopen(xfername, "wt");
      fputs(read_in, xfer);
      fclose(xfer);
      chdir(ra.text_dir);
   }
   if (ch == 'X') {
      xfername[0] = 'X';
      xfername[1] = '\0';
      fgets(read_in, 254, protocol_sys);
      sscanf(read_in, "%s", read_in);
      strcat(xfername, read_in);
      sprintf(menu_item," [%-7s]    ", read_in);
      strcat(xfername, ".bat");
      fgets(read_in, 254, protocol_sys);
      strcat(menu_item, read_in);
      fputs(menu_item, door);
      fgets(read_in,254,protocol_sys);
      
      /* clean up macros */
      doMacros(read_in);
      chdir(ra.bat_dir);
      xfer = fopen(xfername, "wt");
      fputs(read_in, xfer);
      fclose(xfer);
      chdir(ra.text_dir);
   }
}

fclose(up);
fclose(down);
fclose(door);
fclose(protocol_sys);
setSpace( ra.homeDisk, ra.homeUser);

}
#if 0
/***********************************************************************

doBuildResults()  creates the results code datbase for modems

Req: RESULTS.SYS in HOME DIRECTORY

************************************************************************/

doBuildResults()

{
   int i, j, baud_int;
   char baud_token[10], c_string[10], baud_string[15], record_[132];
   FILE *r_sys;
   Db_Obj *results_db;
   char *record_format [] = {
        "code,S",
        "int_val,I",
        NULL
   };

   char *desc [] = {
        "code,S",
        NULL
   };
   
   void *res_rec[] {
      &baud_int;
      &t[30];
   };
   
   struct res_rec *r;
           
   idestroy_db("results");
   
   results_db = icreate_db("results", 0, record_format);
   if(imkindex(results_db, "code,U", desc) != OK) {
     iprterr();
     return;
   }  
   
   if ((r_sys = fopen("RESULTS.SYS", "r")) == NULL) {
      cprintf("\n\rNo RESULTS.SYS File\n\r");
      return(FALSE);
   }
   while (fgets(record_, 132, r_sys) != NULL) {
      if (record_[0] == '#') {
         sscanf (record_, "%s %s %s", baud_token, c_string, baud_string);
         if (strlen(baud_token) > 0 && strlen(c_string) > 0) {
            i = -1;
            while(baud_tokens[++i] != '\0') {
               if (!strcmp(baud_token,baud_tokens[i])) /*found it*/
                  { r->baud_int = baud_values[i];
                    strcpy(r->t, c_string);
                    if(strcmp(c_string, "CONNECT") ){
                      strcat(r->t, " ");
                      strcat(r->t, baud_string);
                    }
                    /* add to the database */
                    if (iaddrec(results_db, NULL, **r) != OK) {
                       iprterr();
                       break;
                    }    
                  }
            }
         }
      }
   }
   iclose_db(results_db);
   return(TRUE);
 }

#endif

/************************************************************************/
/* logInit() indexes userlog.buf             */
/************************************************************************/
logInit(argc)
int argc;
{
int i;
int count;
char getCh();

count = 0;

/* clear ra.logTab */
for (i=0;  i<MAXLOGTAB;  i++) ra.logTab[i].ltnewest  = ERROR;

/* load ra.logTab: */
for (ra.thisLog=0;  ra.thisLog<MAXLOGTAB;  ra.thisLog++) {
   cprintf(" log#%d\r", ra.thisLog);
   getLog(&ra.logBuf, ra.thisLog);
   /* count valid entries:        */
   if (ra.logBuf.lbname[0] == '\0') ra.logBuf.lbvisit[0] = ERROR;
   if (ra.logBuf.lbvisit[0] != ERROR)   count++;
/*   if (ra.logBuf.lbvisit[0] == FALSE  && argc == 1) {
      cprintf(" \n\r %s is HELD.  Kill log? ", ra.logBuf.lbname);
      if ( (char) toUpper(getCh()) == 'Y') {
    ra.logBuf.lbname[0] = ra.logBuf.lbpw[0] = '\0';
    ra.logBuf.lbvisit[0] = ERROR;
    putLog(&ra.logBuf, ra.thisLog);
    }
      } */
   /* copy relevant info into index:   */
   ra.logTab[ra.thisLog].ltnewest = ra.logBuf.lbvisit[0];
   ra.logTab[ra.thisLog].ltlogSlot= ra.thisLog;
   ra.logTab[ra.thisLog].ltnmhash = hash(ra.logBuf.lbname);
   ra.logTab[ra.thisLog].ltpwhash = hash(ra.logBuf.lbpw  );
   }
cprintf("\n\r Init %d logs\n\r ", count);
sortLog();

return TRUE;
}

/************************************************************************/
/* msgInit() sets up lowId, highId, ra.catSector and ra.catChar,  */
/* by scanning over message.buf              */
/************************************************************************/
msgInit() {
unsigned long int firstLo, hereLo;

startAt(0, 0);
getMessage();

cprintf("\n\r Beginning an interminable message scan.");
/* get the ID# */
sscanf(ra.msgBuf.mbId, "%lu", &firstLo);
cprintf("\n\r message# %9u ", firstLo);
ra.newestLo   = firstLo;

ra.oldestLo   = firstLo;

ra.catSector  = ra.thisSector;
ra.catChar = ra.thisChar;

for (
      getMessage();
   
      sscanf(ra.msgBuf.mbId, "%lu", &hereLo),
      !(/*hereHi == firstHi   && */  hereLo == firstLo);
   
      getMessage()
   
       ) {
   cprintf("\r message# %9lu ", hereLo);

   /* find highest and lowest message IDs: */
   /* 32-bit "<" by hand: */
   if (hereLo<ra.oldestLo) {
      ra.oldestLo   = hereLo;
      cprintf("\r  oldest# %9u", ra.oldestLo);
      }
   if ( hereLo>ra.newestLo) {
      ra.newestLo   = hereLo;
      cprintf("\r  newest# %9u", ra.newestLo);

      /* read rest of message in and remember where it ends,  */
      /* in case it turns out to be the last message    */
      /* in which case, that's where to start writing next message*/
      while (dGetWord(ra.msgBuf.mbtext, MAXTEXT));
      ra.catSector  = ra.thisSector;
      ra.catChar = ra.thisChar;
      }
   }
return TRUE;
}

/************************************************************************/
/* sortLog ShellSorts userlog by time of last call       */
/************************************************************************/
sortLog() {
#define TSIZE 10
char *temp[TSIZE];
int finis, i, intCount, step;

cprintf("sortLog\n\r ");
if(ra.sizeLTentry > TSIZE) {
   cprintf(">SIZE to >%d\n\r ", ra.sizeLTentry);
   }

intCount = 0;
for(finis=FALSE, step=MAXLOGTAB >> 1;  !finis || step>1;  ) {
   if (finis) {
      step = step/3 + 1;
      finis = FALSE;
      }

   finis = TRUE;

   cprintf("step=%d\r ", step);

   for(i=step;  i<MAXLOGTAB;  i++) {
      if(ra.logTab[i-step].ltnewest < ra.logTab[i].ltnewest) {
    intCount++;
    finis = FALSE;

    /* interchange two entries */
    movmem(&ra.logTab[i-step], temp, ra.sizeLTentry);
    movmem(&ra.logTab[i], &ra.logTab[i-step], ra.sizeLTentry);
    movmem(temp, &ra.logTab[i], ra.sizeLTentry);
    }
      }
   }
cprintf("\n\r %d swaps\n ", intCount);
return TRUE;
}

/************************************************************************/
/* wrapup() finishes up and writes ctdlTabl.sys out, finally   */
/************************************************************************/
wrapup(argc)
int argc;

{
/* int fd; */

if((/* fd = */ chmod("ctdlTabl.sys",S_IREAD | S_IWRITE)) != -1)
   { 
   unlink("ctdlTabl.sys");
   cprintf("\n\r old ctdlTabl.sys erased\n\r ");
   }
msgInit();
indexRooms();
cprintf("\n\r ");
logInit(argc);
cprintf("writeSysTab =%s\n\r ", (writeSysTab() ? "OK" : "Not Saved"));
return TRUE;
}

/************************************************************************/
/* zapLogFile() erases & re-initializes userlog.buf      */
/************************************************************************/
zapLogFile() {
char getCh();
int  i;

/*cprintf("\n\r Kill log? ");
if ( (char) toUpper(getCh()) != 'Y')   return FALSE;
*/
/* clear RAM buffer out:        */
ra.logBuf.lbvisit[0]   = ERROR;
for (i=0;  i<MAILSLOTS;  i++) {
   ra.logBuf.lbslot[i]= ERROR;
   ra.logBuf.lbId[i] = ERROR;
   }
for (i=0;  i<NAMESIZE;  i++) {
   ra.logBuf.lbname[i]= 0;
   ra.logBuf.lbpw[i] = 0;
   }

/* write empty buffer all over file;  */
for (i=0;  i<MAXLOGTAB;  i++) {
   putLog(&ra.logBuf, i);
   }
return TRUE;
}

/************************************************************************/
/* zapMsgFl() initializes message.buf           */
/************************************************************************/
zapMsgFile() {
char getCh();
int i, sect, val;


/*cprintf("\n\r Kill all msgs? ");
if ( (char) toUpper(getCh()) != 'Y')   return FALSE;
cprintf("\n\r Kick back, it's Miller time... <grin>");*/
/* put null message in first sector... */
ra.sectBuf[0] = '\xFF'; /*   \          */
ra.sectBuf[1] =  '0'; /*    \         */
ra.sectBuf[2] =  '0'; /*     > Message ID "0 1"   */
ra.sectBuf[3] =  '1'; /*    /         */
ra.sectBuf[4] = '\0'; /*   /          */
ra.sectBuf[5] =  'M'; /*   \ Null messsage     */
ra.sectBuf[6] = '\0'; /*   /          */

for (i=7;  i<SECTSIZE;  i++) ra.sectBuf[i] = 0;

lseek(ra.msgfl, 0L, SEEK_SET);
if ((val = _write(ra.msgfl, ra.sectBuf, SECTSIZE)) != SECTSIZE) {
   cprintf("zap fail\n\r ");
   }

ra.sectBuf[0] =  0;
ra.sectBuf[1] =  0;
ra.sectBuf[2] =  0;
ra.sectBuf[3] =  0;
ra.sectBuf[4] =  0;
ra.sectBuf[5] =  0;
ra.sectBuf[6] =  0;

for (sect=1;  sect<ra.maxMSector;  sect++) {
   /*   lseek(ra.msgfl, sect*SECTSIZE, 0); */
   if ((val = _write(ra.msgfl, ra.sectBuf, SECTSIZE)) != SECTSIZE) {
      cprintf("zap fail\n\r ");
      return (val == SECTSIZE);
      }
   }
return FALSE;
}

/************************************************************************/
/* zapRoomFile() erases and re-initilizes room.buf       */
/************************************************************************/
zapRoomFile()
{
char getCh();
int i;
/*
cprintf("\n Zap rooms?");
if ( (char) toUpper(getCh()) != 'Y') return FALSE;
*/
ra.roomBuf.rbflags  = 0;
ra.roomBuf.rbgen = 0;
ra.roomBuf.rbdisk   = 0;
ra.roomBuf.rbuser[0]   = 0;
ra.roomBuf.rbname[0]   = 0;  /* unnecessary -- but I like it...  */
for (i=0;  i<MSGSPERRM;  i++) {
   ra.roomBuf.msg[i].rbmsgNo = ra.roomBuf.msg[i].rbmsgLoc = 0;
   }

cprintf("max=%d\n\r", MAXROOMS);

for (i=0;  i<MAXROOMS;  i++) {
   cprintf("zap %d\r\n", i);
   putRoom(i, &ra.roomBuf);
   }

/* for (i=0; i<15; i++) BIO_waste[i] = 0; */

/* Lobby> always exists -- guarantees us a place to stand! */
ra.thisRoom      = 0      ;
strcpy(ra.roomBuf.rbname, "Lobby") ;
ra.roomBuf.rbflags  = (PERMROOM | PUBLIC | INUSE);
putRoom(LOBBY, &ra.roomBuf);

/* Mail> is also permanent...   */
ra.thisRoom      = MAILROOM  ;
strcpy(ra.roomBuf.rbname, "Mail")  ;
ra.roomBuf.rbflags  = (PERMROOM | PUBLIC | INUSE);
putRoom(MAILROOM, &ra.roomBuf);

/* Aide> also...       */
ra.thisRoom      = AIDEROOM  ;
strcpy(ra.roomBuf.rbname, "Aide")  ;
ra.roomBuf.rbflags  = (PERMROOM | INUSE | BY_INVITE_ONLY);
putRoom(AIDEROOM, &ra.roomBuf);

/* Log> also...         */
ra.thisRoom      = LOGROOM   ;
strcpy(ra.roomBuf.rbname, "Log")   ;
ra.roomBuf.rbflags  = (PERMROOM | INUSE | BY_INVITE_ONLY);
putRoom(LOGROOM, &ra.roomBuf);

return TRUE;
}

