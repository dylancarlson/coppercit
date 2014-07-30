#include "window.h"
#include "stdio.h"
#include "field.h"
#include "keyboard.h"
#include "defaults.h"
#include "help.h"

#define NUM_OF_LINES 36

int CHKDIR(Field fd, int key);
int ANSINUM(Field fd, int key);
int BAUDRATE(Field fd, int key);
int YESNO(Field fd, int key);
int COMCHECK(Field fd, int key);

extern char *version;
char cformat[81];
char cfg_out[80][78];


char *defaults[] = {
   "Noname BBS",
   "Noname",
   "9195551212",
   "Willy Sysop",
   "1",
   "Y",
   "19200",
   "1000",
   "C:\\bbs",
   "C:\\bbs\\msgbase",
   "Y",
   "N",
   "N",
   "N",
   "Y",
   "360",
   "editor",
   "procomm",
   "N",
   "N",
   "4",
   "0",
   "5",
   "3", 
   "7",
   "ATDT",
   "ATX4M0S0=0|",
   "AT|",
   "ATH0|",
   "+",
   "ATH1|",
   "ATA|",
   "Y",
   "30",
   "Y",
   "Y",
   "\0" 
};
   
char *prompt1[] = {
   "Name of BBS: ", /* 0  */
   "Network Name: ", /*  1 */
   "BBS Phone Number: ", /* 2  */
   "Name of Sysop:  ", /*  3 */
   "Modem Port : COM", /*  4 */
   "Port Locked ? ", /*  5 */
   "Highest Baud Rate (300-38400): ", /* 6  */
   "Size of Message Base (in Kilobytes): ", /* 7  */
   "BBS System Directory: ", /*  8 */
   "Message Base Directory: "  /* 9  */
};
char *help1[] = {
 "nodetitle", "nodename", "phoneno", "sysopname", "portno",
 "lockedport", "highbaud", "messagek", "homedisk", "homedisk" };

/*  int *fcheck1[] = {
  NULL, NULL, NULL, NULL, &COMCHECK, &YESNO, &BAUDRATE, NULL, &CHKDIR, &CHKDIR
  }; */       

char *template1[] = {
   "___________________",
   "___________________",
   "(___) ___-____",
   "___________________",               
   "_",
   "_",
   "_____",
   "______",
   "__________________________________________________",
   "__________________________________________________"
};

int range1[] = {
     ASCII, 
     ASCII,
     NUMERIC,
     ASCII,
     NUMERIC,
     ASCII,
     NUMERIC|BLANK,
     NUMERIC|BLANK,
     ASCII,
     ASCII
};

char *prompt2[] = {
   "Allow Users to Establish Accounts? ",  /* 10 */
   "Twit New Users? ", /* 11 */
   "Allow Reading of Messages While Not Logged In ? ", /* 12 */
   "Allow Message Posting While Not Logged In ? ", /* 13 */
   "Allow Users to Create Rooms ? ", /* 14 */
   "Idle Timeout to Disconnect For Logged-in Users (seconds): ", /* 15 */
   "Console Message Editor: ", /* 16 */
   "Console Terminal Program: ", /* 17 */
   "Display \"Hit ENTER\" at Connect: ", /* 18 */
   "Using Doors? " /* 19 */
};
/* int *fcheck2[] = {
   YESNO, YESNO, YESNO, YESNO, YESNO, NULL, NULL, NULL, YESNO, YESNO
   }; */
char *help2[] = {
 "loginok", "twitbit", "readok", "enterok", "roomok", "timeout",
 "msgeditor", "msgeditor", "hitenter", "doors" };
 
char *template2[] = {
   "_",
   "_",
   "_",
   "_",
   "_",
   "____",
   "__________________________________________________",
   "__________________________________________________",
   "_",
   "_"
};
int range2[] = {
    ASCII, ASCII,ASCII,ASCII,ASCII,NUMERIC|BLANK,ASCII,ASCII,ASCII,ASCII
};

   
char *prompt3[] = {
   "Background Color: ", /* 20 */
   "Author Name Color: ", /* 21 */
   "Message Header Color: ", /* 22 */
   "Room Prompt Color: ", /* 23 */
   "Message Text Color: ", /* 24 */
   "Dial Prefix: ", /* 25 */
   "Modem Init String: ", /* 26 */
   "Second Init String: ", /* 27 */
   "Hangup String: ", /* 28 */
   "Command Mode Character: ", /* 29 */
   "Off Hook String: ", /* 30 */
   "Answer Phone String:" /* 31 */
};
/* int *fcheck3[] = {
   ANSINUM, ANSINUM, ANSINUM, ANSINUM, ANSINUM, NULL, NULL, NULL, NULL, 
   NULL, NULL, NULL
 }; */  
char *help3[] = {
  "colors", "colors", "colors", "colors", "colors", "modemstrings",
  "modemstrings", "modemstrings", "modemstrings", "modemstrings",
  "modemstrings", "modemstrings"
};

char *template3[] = {
   "_",
   "_",
   "_",
   "_",
   "_",
   "__________",
   "________________________________________",
   "________________________________________",
   "____________________",
   "____________________",
   "____________________",
   "____________________"
};
int range3[] = {
    NUMERIC, NUMERIC, NUMERIC, NUMERIC, NUMERIC, ASCII, ASCII, ASCII, 
    ASCII, ASCII, ASCII, ASCII
};
char *prompt4[] = {
   "Use AT CMOS Clock: ", /* 32 */
   "Minutes Allowed On: ", /* 33 */
   "Use Message Headlines: ", /* 34 */
   "Use Username To Login: " /* 35 */
};
/*int *fcheck4[] = {
   YESNO, NULL 
 };
 */   
char *help4[] = {
  "atclock", "timeon", "headline", "userlogin"
};

char *template4[] = {
   "_",
   "___",
   "_",
   "_"
};
int range4[] = {
    ASCII, (NUMERIC | BLANK), ASCII, ASCII
};


int row[] = {
  4, 5, 6, 7,  8,  9, 10, 11, 12, 13, 14, 15
};

int lastkey, i, ch;
int wcol=1, wrow=1, width=78, height=23;
Window form, page1, page2, page3, page4;
Field  fd[40];  
FILE *fp;

void ccit_help(char *helpid)

{
Window xhelp;
int width=0, height=0, curscol, cursrow, key;
int kkk;
h_cursor(&curscol, &cursrow);

if(helpid != NULL)
  xhelp = h_open(helpid, 2, 12, width, height);
else
  xhelp = h_open("null", 2, 12, width, height);
  
key = k_getkey();
kkk=w_close(xhelp);

}

int COMCHECK(Field fd, int key)
{
int comport;
void ccit_help();

if(key == _ESC) return(FD_OK);

comport = f_getint(fd);

if ( comport > 0  && comport < 5)
   return(FD_OK);
else
   {    ccit_help("comport");
        return(FD_ERR);
    }
}


int BAUDRATE(Field fd, int key)
{
long rbaud;
void ccit_help();

if(key == _ESC) return(FD_OK);

rbaud = f_getlong(fd);

if ( rbaud == 300 || rbaud == 1200 || rbaud == 2400 || rbaud == 4800 ||
   rbaud == 9600 || rbaud == 19200 || rbaud == 38400)
   return(FD_OK);
else
   {    ccit_help("onlybauds");
        return(FD_ERR);
    }
}


int ANSINUM(Field fd, int key)
{
int ansinum;
void ccit_help();

if(key == _ESC) return(FD_OK);

ansinum = f_getint(fd);

if ( ansinum >= 0  && ansinum < 8)
   return(FD_OK);
else
   {    ccit_help("colors");
        return(FD_ERR);
    }
}


int CHKDIR(Field fd, int key)
{
char dirstr[80];
void ccit_help();

if(key == _ESC) return(FD_OK);

f_getstring(fd, dirstr);
sscanf(dirstr, "%s", dirstr);

if ( dirstr[1] == ':'  &&  dirstr[2] == '\\')
   return(FD_OK);
else
   {    ccit_help("dirhelp");
        return(FD_ERR);
    }
}


int YESNO(Field fd, int key)

{
char str[5];
 
if (key == _ESC) return(FD_OK);
f_getbuffer(fd, str);

if(toupper(str[0]) == 'Y'  || toupper(str[0]) == 'N') return(FD_OK);
  else
    {   ccit_help("yesno");
        return(FD_ERR);
    }
}                


void doPage1(void)
{
 int i;
 
 page1 = w_open(wcol, wrow, width, height);
 d_change(WUMJUST, JUST_CENTER);
 sprintf(cformat, "%s Configurator - Page 1", version);
 w_umessage(page1, cformat);
 w_lmessage(page1, "ESC - Abort   F1 - Help");
 w_clear(page1); 
 for (i=0; i<10; i++) {
   fd[i] = f_create(prompt1[i], template1[i]);
   f_help(fd[i], help1[i]);
   switch (i) {
     case 4: f_validate(fd[i],  COMCHECK, DISABLE); break;
     case 5: f_validate(fd[i],  YESNO, DISABLE); break;
     case 6: f_validate(fd[i], BAUDRATE, DISABLE); break;
     case 8: 
     case 9: f_validate(fd[i],  CHKDIR, DISABLE); break;
   }  
   f_range(fd[i], range1[i]);
   f_append(page1, 1, row[i], fd[i]);
 }
 f_setbuffer(fd[0], &cfg_out[0][0]);
 f_setbuffer(fd[1], &cfg_out[1][0]);
 f_setbuffer(fd[2], &cfg_out[2][0]);
 f_setbuffer(fd[3], &cfg_out[3][0]);
 f_setbuffer(fd[4], &cfg_out[4][0]);
 f_setbuffer(fd[5], &cfg_out[5][0]);
 f_setbuffer(fd[6], &cfg_out[6][0]);
 f_setbuffer(fd[7], &cfg_out[7][0]);
 f_setbuffer(fd[8], &cfg_out[8][0]);
 f_setbuffer(fd[9], &cfg_out[9][0]);
 f_return(fd[9], ENABLE);
 lastkey = f_batch(page1);
 for ( i = 0; i < 10; i++) {
     f_getbuffer(fd[i], &cfg_out[i][0]);
 }
 w_close(page1);    
}

void doPage2(void)
{
 int i;
 page2 = w_open(wcol, wrow, width, height);
 d_change(WUMJUST, JUST_CENTER);
 sprintf(cformat, "%s Configurator - Page 2", version);
 w_umessage(page2, cformat);
 w_lmessage(page2, "ESC - Abort   F1 - Help");
 w_clear(page2 ); 
 for (i=0; i<10; i++) {
   fd[i] = f_create(prompt2[i], template2[i]);
   f_help(fd[i], help2[i]);
   f_range(fd[i], range2[i]);
   switch (i) {
     case 0:
     case 1:
     case 2:
     case 3:
     case 4:
     case 8:
     case 9: f_validate(fd[i], YESNO, DISABLE); break;
   }  
   f_append(page2, 1, row[i], fd[i]);
 }
 f_setbuffer(fd[0], &cfg_out[10][0]);
 f_setbuffer(fd[1], &cfg_out[11][0]);
 f_setbuffer(fd[2], &cfg_out[12][0]);
 f_setbuffer(fd[3], &cfg_out[13][0]);
 f_setbuffer(fd[4], &cfg_out[14][0]);
 f_setbuffer(fd[5], &cfg_out[15][0]);
 f_setbuffer(fd[6], &cfg_out[16][0]);
 f_setbuffer(fd[7], &cfg_out[17][0]);
 f_setbuffer(fd[8], &cfg_out[18][0]);
 f_setbuffer(fd[9], &cfg_out[19][0]);
 f_return(fd[9], ENABLE);
 lastkey = f_batch(page2);
 for ( i = 0; i < 10; i++) {
     f_getbuffer(fd[i], &cfg_out[i+ 10][0]);
 }
 w_close(page2);    
}


void doPage3(void)
{
 int i;
 page3 = w_open(wcol, wrow, width, height);
 d_change(WUMJUST, JUST_CENTER);
 sprintf(cformat, "%s Configurator - Page 3", version);
 w_umessage(page3, cformat);
 w_lmessage(page3, "ESC - Abort   F1 - Help");
 w_clear(page3); 
 for (i=0; i<12; i++) {
   fd[i] = f_create(prompt3[i], template3[i]);
   f_help(fd[i], help3[i]);
   f_range(fd[i], range3[i]);
   switch (i) {
     case 0:
     case 1:
     case 2:
     case 3:
     case 4: f_validate(fd[i], ANSINUM, DISABLE); break;
   }  
   f_append(page3, 1, row[i], fd[i]);
 }
 f_setbuffer(fd[0], &cfg_out[20][0]);
 f_setbuffer(fd[1], &cfg_out[21][0]);
 f_setbuffer(fd[2], &cfg_out[22][0]);
 f_setbuffer(fd[3], &cfg_out[23][0]);
 f_setbuffer(fd[4], &cfg_out[24][0]);
 f_setbuffer(fd[5], &cfg_out[25][0]);
 f_setbuffer(fd[6], &cfg_out[26][0]);
 f_setbuffer(fd[7], &cfg_out[27][0]);
 f_setbuffer(fd[8], &cfg_out[28][0]);
 f_setbuffer(fd[9], &cfg_out[29][0]);
 f_setbuffer(fd[10], &cfg_out[30][0]);
 f_setbuffer(fd[11], &cfg_out[31][0]);
 f_return(fd[11], ENABLE);
 lastkey = f_batch(page3);
 for ( i = 0; i < 12; i++) {
     f_getbuffer(fd[i], &cfg_out[i + 20][0]);
 }
 w_close(page3);    
}

void doPage4(void)
{
 int i;
 page4 = w_open(wcol, wrow, width, height);
 d_change(WUMJUST, JUST_CENTER);
 sprintf(cformat, "%s Configurator - Page 4", version);
 w_umessage(page4, cformat);
 w_lmessage(page4, "ESC - Abort   F1 - Help");
 w_clear(page4 ); 
 for (i=0; i<4; i++) {
   fd[i] = f_create(prompt4[i], template4[i]);
   f_help(fd[i], help4[i]);
   f_range(fd[i], range4[i]);
   switch (i) {
     case 0: f_validate(fd[i], YESNO, DISABLE); break;
     case 2: f_validate(fd[i], YESNO, DISABLE); break;
     case 3: f_validate(fd[i], YESNO, DISABLE); break;
   }  
   f_append(page4, 1, row[i], fd[i]);
 }
 f_setbuffer(fd[0], &cfg_out[32][0]);
 f_setbuffer(fd[1], &cfg_out[33][0]);
 f_setbuffer(fd[2], &cfg_out[34][0]);
 f_setbuffer(fd[3], &cfg_out[35][0]);
 f_return(fd[3], ENABLE);
 lastkey = f_batch(page4);
 for ( i = 0; i < 4; i++) {
     f_getbuffer(fd[i], &cfg_out[i+ 32][0]);
 }
 w_close(page4);    
}
  
int cfg_interact(void)

{
int jj, kk, hfile;
char fbuf[78];

hfile = h_loadfile("ccitcnfg.txt");
h_call(ccit_help);

d_change(FBEEPERR, ENABLE);

for (jj=0; jj<40; jj++)
    for (kk=0; kk<78; kk++) cfg_out[jj][kk]=' ';
    
i = -1; 
if ((fp = fopen("ccitcnfg.sys", "rt")) == NULL) {
   while (*defaults[++i] != '\0') 
         strcpy(&cfg_out[i][0], defaults[i]);
   }
else {   
   while (fgets(fbuf, 77, fp) != NULL)
         strcpy(&cfg_out[++i][0], fbuf);
   fclose(fp);
}
       
 
 form = w_open(wcol, wrow, width, height);
 d_change(WUMJUST, JUST_CENTER);
 sprintf(cformat, "%s Configurator", version);
 w_umessage(form, cformat);
 do {
    w_clear(form);
    w_putsat(form, 15, 6, "F1 - General Page 1");
    w_putsat(form, 15, 8, "F2 - General Page 2");
    w_putsat(form, 15, 10, "F3 - ANSI Default Colors/ Modem Strings");
    w_putsat(form, 15, 12, "F4 - General Page 4");
    w_putsat(form, 15, 15, "F10 - Done");
    ch = k_getkey();
    switch (ch) {
      case _F1 : doPage1(); break;
      case _F2 : doPage2(); break;
      case _F3 : doPage3(); break;
      case _F4 : doPage4(); break;
    }  
 }   
 while ( ch != _F10);  
 w_curson(form);
 w_putsat(form, 15, 19, "Update CCITCNFG.SYS ? ");
 ch = toupper(k_getkey());
  
 if (ch == 'Y') {
    if((fp = fopen("CCITCNFG.SYS", "wt")) != NULL) { 
       for (i = 0; i < NUM_OF_LINES; i++) { 
           fputs(&cfg_out[i][0], fp);
           if (strchr(&cfg_out[i][0], '\n') == NULL) fputc('\n', fp);
       }
    fclose(fp);   
    }   
    else {
       w_clear(form);       
       w_putsat(form, 15, 9, "Unable to Open CCITCNFG.SYS");
       w_putsat(form, 17, 10,"Press A Key To Continue");
       k_getkey();
    }
 w_close(form);
 }
 clrscr();
 return ((ch == 'Y') ? 1 : 0);
} 


  