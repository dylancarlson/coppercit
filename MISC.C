
#include "ctdl.h"

extern char  mIready(), carrDet();
struct logBuffer logBuf;
extern int errno;
static int status_pos[] = {1, 22, 31, 37, 45, 72, 79};
/***********************************************************************
 19 Aug92  JCL - new module
 
 misc.c includes miscellaneous utilities

 set_baud_array() - loads the baud array from RESULTS.SYS
 get_baud_rate()  - takes response code from modem and sets ra.baud
 get_baud()       - handles answering the phone and calls get_baud_rate()
  
************************************************************************/ 



void fileInfo(char *filename)

{
#define DESC_SIZE 46
  int fd;
  FILE *fp;
  char description[DESC_SIZE], *NL='\n', buffer[128], room_[14], sorts[128];
  long fsize;
  struct ftime ft;
  sprintf(room_, "FILEINFO.%03d", ra.thisRoom);
  sprintf(&ra.msgBuf.mbtext[0], "%s  %s", ra.info_dir, room_);
  logMsg();
  format[0] = 0;
  timeout = time(NULL) + ra.timeout;
  if ( (fd = open(filename, O_RDWR)) != -1) {
     sprintf(buffer, "%-13s", filename);
     fsize = filelength(fd);
     sprintf(description, " %7ld * ", fsize);
     strcat(buffer, description);
     if(getftime(fd, &ft) == 0) {
       sprintf(description, " %02u/%02u ",
               ft.ft_month, ft.ft_day, ft.ft_year+1980);
       strcat(buffer, description);
     }  
     close(fd);
     getString(" Description: ", description, (int) (sizeof(description) - 1));
     if(!strlen(description)) strcpy(description, 
                                     "===> No Description Given <===");
     strcat(buffer, description);

     chdir(ra.info_dir);
     if ((fp=fopen(room_, "a+t")) != NULL) {

        fprintf(fp, "%s\n", buffer);
        fclose(fp);
        sprintf(sorts, "sortf %s temp.srt", room_);
        if (!system(sorts)){ 
        
           unlink(room_);
           sprintf(sorts, "rename temp.srt %s", room_);
           system(sorts);
           }
        else
           doStatusLine(S_MSGS, "FILEINFO Sort Failed");
     }
     else doStatusLine(S_MSGS, "Can't Open FILEINFO");
  }
}
#if 0
 int set_baud_array()
 
 {
   long a;
   int i, j;
   char t[30], baud_token[10], c_string[10], baud_string[15], record_[132];
   FILE *r_sys;
   struct baud_packet *bp_tmp, *hold;
      
   if ((r_sys = fopen("RESULTS.SYS", "r")) == NULL) {
      cprintf("\n\rNo RESULTS.SYS File\n\r");
      return(FALSE);
   }

   a = sizeof(struct baud_packet);
   
   hold = baud_list = malloc(a);
   hold->baud_int = NULL;
   hold->bp_next = NULL;

   while (fgets(record_, 132, r_sys) != NULL) {
      if (record_[0] == '#') {
         sscanf (record_, "%s %s %s", baud_token, c_string, baud_string);
         if (strlen(baud_token) > 0 && strlen(c_string) > 0) {
            i = -1;
            while(baud_tokens[++i] != '\0') {
               if (!strcmp(baud_token,baud_tokens[i])) /*found it*/
                  { hold->baud_int = baud_values[i];
                    strcpy(t, c_string);
                    if(!strcmp(c_string, "CONNECT") ){
                      strcat(t, " ");
                      strcat(t, baud_string);
                    }
                    strcpy(hold->string, t);
                    bp_tmp = malloc(a);
                    hold->bp_next = bp_tmp;
                    hold = bp_tmp;
                    hold->baud_int = -1;
                    hold->bp_next = NULL;
                  }
            }
         }
      }
   }
   return(TRUE);
 }
#endif
 /*******************************************************************
 
  get_baud_rate() takes the result code gotten from the modem and
  determines the baud_rate from the data stored via set_baud_array
  from RESULTS.SYS
  
 ********************************************************************/ 
 void get_baud_rate(char *bd)

 { 
 
   char connect[10], brate[40];

   void initPort(), deinit_fossil();
   int ok, status;
   FILE *fp;
   


   while((*bd < 33 && *bd > 0) || *bd > 0x7A) bd++;
   sToUpper(bd);

     if (ra.debug) {
        if ((fp=fopen("modem.dbg", "a+t")) != NULL) {

        fprintf(fp, "%s\n", bd);
        fclose(fp);
        }
     }   
        
     if (strstr(bd, "FAX") != NULL) {
        deinit_fossil(); 
        setSpace(ra.homeDisk, ra.homeUser);
        ok = gettext( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

        status = system("CITFAX"); 
        clrscr();
        if(ok)
          puttext( 1, (int) ra.ti.screenheight,
            (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

        setSpace(ra.homeDisk, ra.homeUser);
        initPort();
        strcpy(&ra.msgBuf.mbtext, "FAX Connection. See FAX S/W Log");
        logMsg();
        givePrompt();
        ra.baud = MDM_OK;
        }

   if(strstr(bd, "RING") != NULL){
     ra.baud = RING;
     return;
     }
     else if (strstr(bd, "CONNECT") != NULL) {
          sscanf(bd,"%s %s", connect, brate);
          if (isdigit(brate[0]))ra.baud = atol(brate);
          else ra.baud = 300; /* for those modems that send 
                               just CONNECT at 300 baud */
          }
          else if (strstr(bd, "OK") != NULL) {
               ra.baud = MDM_OK;
               return;
               }
               else if (strstr(bd, "ERROR")) {
                    ra.baud = MDM_ERR;
                    return;
                    }
                    else {
                         ra.baud = MDM_OK;
                         return;
                         }
            
            
      
/*   if(ra.baud > B9600) strcpy(ra.baud_asc, "HISPEED");
   else
      if(ra.baud == -1) strcpy(ra.baud_asc, "UNKNOWN");
      else  */
         ltoa(ra.baud, ra.baud_asc, 10);
         
   doStatusLine(MSPEED, ra.baud_asc);

 }

/******************************************************************

 get_baud() is called by modIn() to answer phone and return baud rate

*******************************************************************/

 void get_baud()
 
 { int i, c;
   void initPort();
   void deinit_fossil();
   char s[50 /*16*/];
   
   i = 0;
     
   while( c = receive(3),   ((c & 0x7f) != 13) &&  
        (i<50 /*15*/) && (c != ERROR)) 
           s[i++] = c & 0x7f;

     s[i] = '\0';

   if(ra.debug)
     cprintf("\n\r Modem Spat: %s Length of: %d\n\r ", s, strlen(s));
     
        
     sToUpper(s);
        
     /* regurgitating the cmd string */     
     if( ((s[0] == 'A' && s[1] == 'T') || s[0] == '\0') )
        ra.baud = MDM_OK;
     else        
        get_baud_rate(s);

     /* all this shit and the regurgitation down below is to allow */
     /* CONNECT XXXX that precede the character string with CRLF   */
     if (s[0] == '\0' || (s[0] < ' ' && s[1] == '\0') )
        ra.baud = MDM_OK;
     else
        get_baud_rate(s);

     if (ra.baud == MDM_OK) {
        doStatusLine(S_MSGS, "   OK    ");
        ra.baud = -1; 
        return 0;
        }
     else
        if (ra.baud == MDM_ERR || ra.baud == -1) {
           doStatusLine(S_MSGS, "Mdm Error");
           ra.baud = -1; 
           return 0;
        }
           
     if (ra.baud == RING) {  /* aha! Hayes result code for a RING */
        doStatusLine(S_MSGS, "   Ring  ");
        pause(40);
        doAnswerPhone();   /* tell modem to answer the phone    */

      do {
         i = 0;

          while( c = receive(17), ((c & 0x7f) != 13) 
                 && (i<50 /*15*/) && (c != ERROR))
               s[i++] = c & 0x7f;

          s[i] = '\0';

   if(ra.debug)
     cprintf("\n\r Modem Spat at 2: %s Length of: %d\n\r ", s, strlen(s));
          if (s[0] == '\0' || (s[0] < ' ' && s[1] == '\0') )
             ra.baud = RING;
          else
             get_baud_rate(s);
         }
      while( ra.baud == RING ); /* just in case another RING snuck in */

      if(!ra.lockedport)
        {
         switch(ra.baud){

         case B300:  /* result code for 300 baud   */
              set300();
              break;

         case B1200:  /* result code for 1200 baud  */
              set1200();
              break;

         case B2400: /* result code for 2400 baud  */
              set2400();
              break;

         case B4800: /* result code for 4800 baud */
              set4800();
              break;

         case B9600:  /* result code for 9600 baud */
              set9600();
              break;
      
/*         case HISPEED: 
              set19200();
              break;
*/      
         default:
              if (ra.baud > ra.hibaud || ra.baud < B300)
                  pull_DTR();  /* screwup */
              else
                  setbaud (ra.hibaud);    
              break;
      }
    }
   }
   else{
      ra.baud = -1;
      hangUp(); /* modem mess up?? */
   }
   if (ra.debug) cprintf("\n\r Cit baud = %d \n\r", ra.baud);
}


/*****************************************************************************
 *                           UserMap                                         *
 *****************************************************************************
 *                                                                           *
 *  This program will cross reference all users with the hidden rooms each   *
 *  has access to.                                                           *
 *  92Sep09 JCL  Moved to Borland C++ 2.0 for MSDOS                          *
 *  85Aug01 |br| v2.0 - taken a step beyond as a 'logsweep' a la NSWEEP      *
 *  85Jul27 |br| v1.2 - some mods to give other info besides name            *
 *  84Nov18 |br| v1.11- added some couth - eliminated blank log entries      *
 *  84Nov14 TGM  v1.1 - added single name modification.                      *
 *  84Nov13 TGM  v1.0 created                                                *
 *                                                                           *
 *****************************************************************************/

#define         ROOMS_PER_LINE  6

umap()
{
    int roomfile, logfile;
    char        done, answer, name[NAMESIZE];

    done        = FALSE;                /* setup flags                  */
    p_port      = FALSE;

    hello();
    setSpace(ra.homeDisk, ra.homeUser);
    roomfile = open("CTDLROOM.SYS", O_RDONLY);
    loadrooms(roomfile);                /* go fill the room table       */
    close(roomfile);            /* why not we are done with it  */

    while (TRUE) {                      /* main command loop            */
        while (!done) {                 /*      command loop            */
            name [0] = 0;
            mPrintf("\nEntire list, QUIT, or Single name (E/Q/S)? ");

            switch (answer = toupper(iChar())) {
            case CNTRLa:
                if (ra.onConsole) {
                  p_port = TRUE;         /* turn printer flag on         */
                  mPrintf(" printer on\7"); /* tell him what he's done     */
                }  
                break;

            case 'E':                   /* Entire List                  */
                done    = TRUE;
                break;

            case 'Q':                   /* QUIT                         */
            case SPECIAL :              /* ESCAPE                       */
                p_port = FALSE;
                return;                 /* dats all folks!              */

            case 'S':                   /* Single person on list        */
                getString(" Name of User: ", name, NAMESIZE);
                done    = TRUE;
                break;

            }
        }
        logfile = open("CTDLLOG.SYS",  O_RDONLY);
        entire_ulog(logfile, name);
        close(logfile);
        done    = FALSE;
        p_port = FALSE;
    }
}

/***********************************************************************
List entire userlog
************************************************************************/
entire_ulog(logfile, name)
int     logfile;
char    *name;
{
        int     i, one_flag;

        lseek(logfile, 0, 0);
        
        mPrintf("\n\n");
        one_flag = name[0];
        for (i = 0; i < MAXLOGTAB; i++) {
                _read (logfile, &logBuf, SECSPERLOG);   /*get a log entry*/
                if (!one_flag) strcpy (name, logBuf.lbname);
                if (strlen(logBuf.lbname)
                        &&
                   (hash(logBuf.lbname) == hash(name))) {     /*real person? */
   sprintf(format,"%03d: %-20s %3d m%5lu %2s %2s %2s %2s %2s %2s %2s (%s)\n",
                                i,logBuf.lbname,
                                (int) logBuf.lbwidth,
                                logBuf.lbvisit[0],
                                (logBuf.lbflags & AIDE          ? "AD" : "  "),
                                (logBuf.lbflags & TWIT          ? "TW" : "  "),
                                (logBuf.lbflags & UCMASK        ? "UC" : "  "),
                                (logBuf.lbflags & LFMASK        ? "LF" : "  "),
                                (logBuf.lbflags & TABMASK       ? "TB" : "  "),
                                (logBuf.lbflags & EXPERT        ? "EX" : "  "),
                                (logBuf.lbnulls & SYSOP         ? "RS" : "  "),
                                logBuf.lbpw);
                        mPrintf(format);
                        check_for_rooms();  /* print accessible rooms */
                        if (one_flag) return;   /* done, why stick around!*/
                }
        if (mAbort() /*KBReady() && (getch() == SPECIAL)*/)
           return;  /* escape to abort */
        }
}
/****************************************************************************  
 check_for_rooms will check the current user log entry against all of the
 room entries and flag the rooms user has access to
 *****************************************************************************/
check_for_rooms()

{          int     count, roomline;
           char foundRoom;
           
           foundRoom = FALSE;
           roomline = 8;
           
           for (count = 0; count < MAXROOMS; count++) {
                if (           /*display only inuse, hidden rooms!*/
                    (ra.roomTab[count].rtflags & INUSE)
                    &&
                    !(ra.roomTab[count].rtflags & PUBLIC)
                    &&
                    ra.roomTab[count].rtgen == (logBuf.lbgen[count] >> GENSHIFT)
                   ) {
                        foundRoom       = TRUE;
                        roomline += strlen(ra.roomTab[count].rtname);
                        if (roomline > 79 ) {
                            roomline = 8 + strlen(ra.roomTab[count].rtname);
                            mPrintf ("\n\t");
                        }
                        sprintf (format,"%s  ",ra.roomTab[count].rtname);
                        mPrintf(format);
                
                }
        }
        if (foundRoom)                  mPrintf("\n");
        mPrintf("\n");
}

/************************************************************************
 *      loadrooms: load INUSE rooms into room table                     *
 ************************************************************************/
loadrooms(roomfile)
int     roomfile;
{
        int     i;

/*        for (i = 0; i < MAXROOMS; i++) {
                _read (roomfile, roomBuf, SECSPERROOM);          /*get a room*/
                roomTab[i].rtflags = roomBuf.rbflags;       /*save the flags*/
                roomTab[i].rtgen = roomBuf.rbgen;          /*save generation*/
                strcpy (roomTab[i].rtname, roomBuf.rbname);       /*and name*/
        }
*/    /* already loaded */        
}

/************************************************************************/
/*  say HELLO to the user, tell the poor soul just what's going on...   */
/************************************************************************/
hello()
{ char ch;

  clrscr();
  mPrintf("\n\n\t\t\tCitadel  Usermap");
  mPrintf("\n\t\t\t-------  -------");
  mPrintf("\n\n\t\tTom Marazita, 84Nov13, Lobo Max-80 Citadel");
  mPrintf("\n\t\tBrian Riley, 85Aug01, Morningstar Keep Citadel");
  mPrintf("\n\n\tThis simple utility will display a list of the userlog along with");
  mPrintf("\n\tthe respective hidden rooms a user has access to.\n");
  mPrintf("\n\tFrom the command prompt a cntrl-A will toggle printer on during");
  mPrintf("\n\texecution of that command cycle, either single or entire and then");
  mPrintf("\n\treset the flag for the next.(BBR)\n ");
  mPrintf("\n ");
  doCharPause(1);
  /* mPrintf("-Pause-");
  ch = iChar(); */  
}

/*************************************************************************

 callerlog()  reads the caller log from end to start

 *************************************************************************/

 callerLog()

 {
 long a, f, number;
 FILE *callfl;
 char buffer[78];
 setSpace(ra.homeDisk, ra.homeUser);
 
 callfl = fopen("CALLLOG.SYS", "r+t");
 rewind(callfl);
 
 a = sizeof(struct record_53);

 /* check for empty file */
 if((f = filelength(ra.callfl)-1) < (a - 1))
   { fclose(callfl);
    return;
   } 
 
 doCR();

 number = ra.callerno - (ra.loggedIn ? 1 : 0);

 sprintf(format, "\n There Have Been %ld Previous Logins\n ", number);
 mPrintf(format);
 doCR();
   
 while ( (fgets(buffer, 78, callfl) != NULL)  && !mAbort()) {
   sprintf(format, " #%6ld    ", number--);
   mPrintf(format);
   if (ra.aide) mPrintf(buffer);
   else mPrintf(&buffer[18]);
   doCR();
   }

 fclose(callfl);
 
 }


/**************************************************************************

 fix_rollover()    a Q&D routine to make sure MSDOS gets the date correct
                   after midnight
                   
**************************************************************************/

fix_rollover()

{
struct date d;
struct time t;
struct tm *tb;
time_t timer;
int fhour, fdate, fminute;
void initPort();

if (!ra.AT) {
   gettime(&t);
   getdate(&d);
   fdate = (int) d.da_day;
   fminute = t.ti_hour;
   if( (fhour = t.ti_hour) != 0) fix_flag = 1;
   }
else
      
   {

   timer = time(NULL);
   tb = localtime(&timer);
   fdate = tb->tm_mday;
   fminute = tb->tm_min;
   if( (fhour = tb->tm_hour) != 0) fix_flag = 1;

  }
   if (fix_flag && !fhour) {  /* we've passed midnite */
      fix_flag = 0;
      if(fdate == ra.date) {  /* date did not roll */
      fix_date = ra.date + 1;
      if (fix_date > 28 && fix_month == 2) {
         if ( !fix_year && !( fix_year % 4) || fix_date > 29 ) {
            fix_month = 3;
            fix_date  = 1;
         }
      else {
         if (fix_date == 31) {
            switch (fix_month) {
               case 9:
               case 4:
               case 6:
               case 11:
                 fix_date = 1;
                 fix_month += 1;
                 break;
             }
         }
         else if (fix_date == 32) {
                fix_month += 1;
                fix_date   = 1;
              }
        }
        if (fix_month == 13) {
           fix_month = 1;
           fix_year  += 1;
           if (fix_year == 100) fix_year = 0;
        }
     }
     d.da_year = fix_year + ((fix_year < 91) ? 2000 : 1900);
     d.da_day  = (char) fix_date;
     d.da_mon  = (char) fix_month;
     setdate(&d);

    ra.year = fix_year;
    ra.month = fix_month;
    ra.date = fix_date;
    }
  }

if( fminute != save_minute) {  
  sprintf(format, "%02d:%02d", fhour, fminute);
  doStatusLine( CURR_TIME, format);
  save_minute = fminute;
}  

if(!fminute && !init_modem_done) init_modem_hourly = 1;
if(fminute  && init_modem_done) {
  if(ra.haveCarrier) doStatusLine( S_MSGS, "                         ");
  else doStatusLine(S_MSGS, "Waiting...");
  init_modem_done   = 0;
  }
  
if(init_modem_hourly && !ra.haveCarrier && !ra.loggedIn 
   && !is_door && !is_mailer)
  {
   initPort();
   init_modem_hourly = 0;
   init_modem_done   = 1;
  }
}

void setStatusLine()
{
  int i;
  char ch[133];
  
  clrscr();
  window( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight);
  textcolor(1);
  textbackground(7);
  /* since Desqview Windows have problems with fields made up of all
     spaces, we must write characters then blank them out  */
  gotoxy(1,1);
  for (i = 1; i <= (int) ra.ti.screenwidth; i++) 
    /* cprintf("-");*/ ch[i-1] = 32;
    ch[i-1] = 0;
 
  gotoxy(1, 1);
/*  for (i = 1; i <= (int) ra.ti.screenwidth; i++) 
      cprintf("%c", ch);*/
      
  cprintf("%s", ch);

  initStatusLine();
    

 /* set BBS window */
  window( 1, 1,(int) ra.ti.screenwidth, (int) (ra.ti.screenheight - 1));
  textcolor(11);
  textbackground(1);
  clrscr();
  gotoxy(1, 1);
}



doStatusLine (int which_field, char *status_message)

{
  struct text_info r;
  int i;
  
  if(blank_flag) return;

  gettextinfo(&r);
  window( 1, (int) ra.ti.screenheight,
          (int) ra.ti.screenwidth, (int) ra.ti.screenheight);

  textcolor(1);
  textbackground(7);

  gotoxy(status_pos[which_field], 1);

  switch (which_field) {
    case USER      : cprintf("%20s", status_message); break;
    case MSPEED    : if(!carrDet()) strcpy(status_message, "       ");
                     cprintf("%8s" , status_message); break;
    case T_LOGON   : cprintf("%5s" , status_message); break;
    case WHICH_IO  : cprintf("%7s" , status_message); break;
    case S_MSGS    : cprintf("%26s", status_message); break;
    case CURR_TIME : cprintf("%5s" , status_message); break;
    case INIT_LINE : initStatusLine();                break;
    case T_CHAT    : putch(*status_message);          break;
  }
    
  window((int) r.winleft, (int) r.wintop, 
         (int) r.winright, (int) r.winbottom);
  textcolor(11);
  textbackground(1);
  gotoxy((int) r.curx, (int) r.cury);
}
 
initStatusLine()

{
 /* initialize the status line */

  sprintf(format, "%19s", version);
  doStatusLine(USER, format);
  doStatusLine(MSPEED, "        ");
  doStatusLine(T_LOGON, "     ");
  doStatusLine(WHICH_IO, " MODEM ");
  doStatusLine(S_MSGS, " Waiting... ");
  format[0] = (ra.noChat ? ' ' : 'C');
  doStatusLine(T_CHAT, format);
  StatusLineDate();
}

StatusLineDate()
{
  getDaTime();
  sprintf(format, "%02d:%02d", ra.hour, ra.minute);
  doStatusLine(CURR_TIME, format);
}

doCharPause(int a)

{
  char ca, cflag;
  long  cb;
  int cntr, ch;
  char *pauseline="\n - <S>top <N>ext <C>ontinuous <Enter> -";
  
  if(a) {
    ra.crtColumn = 1;
    mPrintf(pauseline);
    outMod('\r');
    putch('\r');
    }
    
  timeout = time(NULL) + (ra.loggedIn ?  ra.timeout :  ra.timeout/4);   
  do {
    cflag = (time(NULL) < timeout);
    if (cflag){
         if (!ra.onConsole) cflag = (!mIready() && carrDet()) ;
         else cflag = ( !KBReady() );
         }
  }
  while ( cflag );
  
  if(mIready() && carrDet() ) ch = (int) getMod_Peek() & 0x7F;
  else  ch=(bioskey(1) & 0x7F);

  if(ch == 'C'  || ch == 'c') pause_override = TRUE;
  scrn = 0;
  if(a) for (cntr = 0; cntr < strlen(pauseline); cntr++) {
    outMod(' ');
    putch(' ');
    }
  if (a) {
  if(a) for (cntr = 0; cntr < strlen(pauseline); cntr++) outMod('\b');
/*    outMod('\r'); */
    putch('\r');
    }  
  ra.crtColumn = 1;
}

doANSI(int i)

{
char bffr[80], *ch;
ch = bffr;
sprintf(bffr,"%c[0;1;%02dm", ESC, i);
while(*ch != '\0') outMod(*ch++);

}

/*********************************************************************
 Protocol Stuff:
 
 doXfer()         ;  Handles file transfers via external protocols
 
 **********************************************************************/

/*****************************************************/
       
doXfer(char direction)

{
char filedr[40],system_call[128], choice, fileName[20], *s, menu[60];
int i, j, statis;

static unsigned long cps[] = { 27, 108, 216, 432, 864, 1728, 3456};
int fileDir(), ok, packet_size, attr;
unsigned long dlT;
void initPort();
void deinit_fossil();
struct ffblk filblk;

doCR();
setSpace(ra.homeDisk, ra.homeUser);
strcpy(menu, ra.text_dir);
if (direction == 'U') tutorial("UPMENU.SYS");
else tutorial("DOWNMENU.SYS");
doCR();
mPrintf("Enter Selection ===> ");
choice = toupper(iChar());
if ( choice <= ' ') return;
doCR();
 statis = 0;
 j=1;
 
 getString("filename", fileName, NAMESIZE);

 if ( !strlen(fileName) ) return;
 if (!carrDet()) return;
 
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

sprintf(system_call, "%s\\%c%c  %d %ld %ld ", ra.bat_dir,
        direction, choice,  ra.portno+1,
        ra.baud, ra.hibaud);
sprintf(filedr,"%c:%s\\", ra.roomBuf.rbdisk+'A', ra.roomBuf.rbuser);
strcat(system_call, filedr);
strcat(system_call, fileName);
sprintf(filedr," %c: %s %s %s", ra.roomBuf.rbdisk+'A', ra.roomBuf.rbuser,
        fileName, ra.logBuf.lbname);
strcat(system_call, filedr);
        
cprintf("%s\n\r",system_call);
 if(direction == 'D'){  /* transmit file */
    ra.FDSectCount = 0;
    if(strLen(fileName))  { 
       wildCard(fileDir, fileName); 
      }
    else           {  
       return; 
    }
   
    if (ra.FDSectCount) { 
      doCR();
      if (ra.baud > 0 && ra.baud < RING)
         dlT = 1 + ( (ra.FDSectCount * 9) / ra.baud );

      sprintf(format, "%ld bytes takes approx. %ld min. %2ld sec.",
              ra.FDSectCount, dlT/60, dlT%60);
      mPrintf(format);  
      if ( !ra.aide && ((dlT/60) > ((ra.time_allowed + 15) - timeOn())) ){
         mPrintf("Not enough time left in session");
         return;
      }
     setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);


     if(!carrDet()) return;

     if ( choice == 'X') {
        packet_size = 128;
        if(getYesNo("Xmodem-1K")) packet_size = 1024;
        if (!getYesNo("Ready"))  return;
        ra.usingWCprotocol = TRUE;
        xsendfile(fileName, packet_size);
        ra.usingWCprotocol = FALSE;
      }
      else { 
             if (!getYesNo("Ready"))  return;
             ok = gettext( 1, (int) ra.ti.screenheight,
                (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
             deinit_fossil();
             statis = system(system_call);
             if(ok)
               puttext( 1, (int) ra.ti.screenheight,
                  (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);
             no_cit_init = TRUE;
             initPort();
             
       }

       
     
     clrscr();
     
     no_cit_init = FALSE;
     sPrintf(ra.msgBuf.mbtext, "%s from %s by %s",
        fileName,
        ra.roomBuf.rbname,
        ra.logBuf.lbname);
     logMsg();
     setSpace(ra.homeDisk, ra.homeUser);

    }
    else return;
  } /* endif whichway == send */
  
  else {  /*receive file */
       
   s = &fileName[0];
   setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);


/*   if((upld = open(s, O_RDONLY | O_BINARY)) != -1) { */
     if(findfirst(s, &filblk, attr) == 0) {
      mPrintf("\n That file exists here\n\x07 ");
      close( upld ); 
      return;
   } 
   else {
     sleep(2);
     if (!ra.expert) {
        tutorial("wcupload.blb");
     }

   getDfr();
   if ((!ra.freeSpace)||
      (( upld = open(s, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE)) == -1)){
         mPrintf("\n Full Disk or Bad Character in Filename");
         unlink(s);
         setSpace (ra.homeDisk, ra.homeUser);
         return;        
   }    
   else {
      /* get space available */
      sprintf(format, "%lu kb Free:",ra.freeSpace);
      mPrintf(format);  
      if(!carrDet()) return;
      /* then upload         */
     {  
        if (choice == 'X') {
           statis = xreadFile( upld );
           close( upld );
           if (statis) fileInfo(fileName);
           else unlink(s);
        }
        else {        
           ok = gettext( 1, (int) ra.ti.screenheight,
             (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

           close(upld);
           if (!getYesNo("Ready"))  return;

           deinit_fossil();
           system(system_call);
           
           
           if(ok)
             puttext( 1, (int) ra.ti.screenheight,
               (int) ra.ti.screenwidth, (int) ra.ti.screenheight, format);

           clrscr();
           no_cit_init = TRUE;
           initPort();
           no_cit_init = FALSE;
           
           statis = TRUE; /* successful upload by default */
           
           if((upld = open(s, O_RDONLY | O_BINARY)) == -1) {
              return;
              }
           else {
              if (filelength(upld) == 0) {
                 statis = FALSE;
                 close(upld);
                 }
              else close(upld);
           }      
           if (!statis) unlink(s);
           else fileInfo(fileName);
           }
   } 

        setSpace (ra.homeDisk, ra.homeUser);
        sPrintf(ra.msgBuf.mbtext, "%s to %s by %s",
        fileName,
        ra.roomBuf.rbname,
        ra.logBuf.lbname);
        if(statis) logMsg();
         
    }
   }
 }
}

/************************************************************

 doMacros expand CopperCit Macros
 
*************************************************************/
 
doMacros(char *buf)

{

char *ptr;


while ((ptr = strchr( buf, '$')) != NULL) {

   *ptr++ = '%';
   switch ( toupper(*ptr) ) {
      case 'C' : *ptr = '1'; break;
      case 'B' : *ptr = '2'; break;
      case 'L' : *ptr = '3'; break;
      case 'F' : *ptr = '4'; break;
      case 'D' : *ptr = '5'; break;
      case 'P' : *ptr = '6'; break;
      case 'N' : *ptr = '7'; break;
      case 'U' : *ptr = '8'; break;
   }
 }
}

/************************************************************

 doDoors() calls the doors menu and calls that external program
 
*************************************************************/
 
doDoors()

{
char choice[8], system_call[128];
FILE *fp;

tutorial("doormenu.sys");

getString(" Selection ======> ", choice, 7);

if(strlen(choice) == 0) return;

sscanf(choice, "%s", choice);
sToUpper(choice);
sprintf(system_call, "%s\\X%s  %d %ld %ld \"%s\" \n", ra.bat_dir,
        choice,  ra.portno+1,
        ra.baud, ra.hibaud, &ra.logBuf.lbname);
setSpace(ra.homeDisk, ra.homeUser);
cprintf("%s\n", system_call);
fp = fopen("DROPFIL.BAT", "w+b");
fputs(system_call, fp);
fclose(fp);

doDorinfo();
writeSysTab();
choice[0] = 0;
if (is_door) {
   choice[0] = 'D';
   ltoa(ra.baud, &choice[1], 10);
   }
   else if(is_mailer) {
        choice[0] = 'M';
        ltoa(ra.baud, &choice[1], 10);
        }
setSpace(ra.homeDisk, ra.homeUser);
unlink("CCIT.LCK");        
execlp("ccitdoor.exe", "ccitdoor.exe",  choice, NULL);
        
 
}

/********************************************************************

doDorinfo creates the dorinfo1.def drop file

*******************************************************************/

doDorinfo()
{
char first_name[20], *last_name, n_title[21], *OA;
FILE *fp;

fp = fopen("DORINFO1.DEF", "w+t");
sscanf(ra.sysopname, "%s", first_name);
last_name = ra.sysopname + strlen(first_name);
strcpy(n_title, ra.nodeTitle);
OA = strchr( n_title, '\x0a');
if (OA != NULL) *OA = 0;
fprintf(fp, "%s\n", n_title);
fprintf(fp, "%s\n", first_name);
if (*last_name) {
  while (*last_name == ' ' && *last_name != 0) last_name++;
  fprintf(fp, "%s\n", last_name);
  }
else fprintf(fp, "   \n");
fprintf(fp, "COM%d\n", ra.portno + 1);
fprintf(fp, "%ld BAUD,n,8,1\n", (ra.lockedport ? 
       ra.hibaud : ra.baud));
fprintf(fp, "0\n");
sscanf(ra.logBuf.lbname, "%s", first_name);
last_name = ra.logBuf.lbname + strlen(first_name) + 1;
fprintf(fp, "%s\n", first_name);
if ( *last_name) {
   while (*last_name == ' ' && *last_name != 0) last_name++;
   fprintf(fp, "%s\n", last_name);
   }
else fprintf(fp, "   \n");
fprintf(fp, "NORTH CAROLINA\n");
fprintf(fp, "%d\n", ra.ansi_on ? 1 : 3 );
fprintf(fp, "0\n");
fprintf(fp, "%d\n", (ra.time_allowed - timeOn() ) );
fprintf(fp, "0\n");
fflush(fp);
fclose(fp);

}

/**************************************************

doSystemMsgs() - hands out bulletins

**************************************************/

doSystemMsgs()

{
 char filename[15];
 int  bullno = 0, chk;
 
 do {
     doCR();
     doCR();
     sprintf(filename, "BULLETIN.%03d", bullno++);
     chk = (int) tutorial(filename);
     if (ra.termMore && chk != ERROR) doCharPause(1);
 } while( !mAbort() &&  chk != ERROR );

 ra.outFlag = OUTOK;             
}
   
 