/************************************************************************/
/* msdos     8/91             Ctdl.h                                    */
/*                                                                      */
/* #include file for all Citadel C files.                               */
/************************************************************************/


/************************************************************************/
/*          History                                                     */
/* 86Mar12 .bp. Changed special to effect filter                        */
/* 83Mar03 CrT Changed from 2 to 3 sectors / log-record (more mail)     */
/* 83Feb23  termTab, TABMASK                                            */
/* 82Dec16 dvm Place for #defines added                                 */
/* 82Dec06 CrT 2.00 release.                                            */
/* 82Nov02 CrT General cleanup begun, preparatory to V1.2 mods.         */
/* 82Nov01 CrT Proofread for distribution version.                      */
/* 82Oct05 dvm distribution version mods begun                          */
/* 82Jun03 dvm MAXROOMS reduced to 64                                   */
/* 82Apr22 dvm mods for HiTech/NMI version completed                    */
/* 81Dec19 CrT Rooms seem to be working...                              */
/* 81Dec12 CrT Started.                                                 */
/************************************************************************/

/************************************************************************/
/*          #defines    82Dec10 dvm                                     */
/*                                                                      */
/* #defs supported in current system:                                   */
/*                                                                      */
/* #def FDC-1 for the TeleTek (dvm code)                                */
/* #def VFC-2 for the Big Board (dvm code)                              */
/*                                                                      */
/* #def TEST for special small-scale test version                       */
/************************************************************************/
/*#define char unsigned char*/
#include <conio.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <time.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <process.h>
#include <dir.h>

struct timePacket {
        long tPday, tPhour, tPminute, tPsecond, tPmilli;
} ;


struct baud_packet {
    int baud_int;
    char string[30];
    struct baud_packet *bp_next;
    };

struct record_53 {
        char time_day[18];
        char name_baud[28];
        char timeOn[15];
        char cr;
        char lf;
      };

struct xfer_protocol {
        char menu_key;
        char menu_item[39];
        char menu_call[80];
      };


typedef struct ramarea {
    char firstExtern ; /* this should ALWAYS be the first declaration   */

    int  megaHz;      /* Z80 clock rate (for wait loops)                */
    int  lockedport;  /* for high speed modems - 0=no, 1=yes            */
    int  portno;      /* 0 = COM1, 1 = COM2, etc.                       */
    char nodeName[21];
    char nodeTitle[21];
    char nodeId[21];
    char sModStr[41];
    long  hibaud;


    char rcpm;        /* TRUE to run in RCP/M mode                      */
    char homeDisk;
    char homeUser[60];
    char msgDisk;     /* where we keep our data files                   */
    char msgUser[60];
    char ourDisk;
    char ourUser[60];        /* where we are just now   */

    char unlogEnterOk ;     /* TRUE if OK to enter messages anon   */
    char unlogReadOk  ;     /* TRUE if unlogged folks can read mess */
    char unlogLoginOk ;     /* TRUE if spontan. new accounts ok.   */
    char nonAideRoomOk;     /* TRUE general folks can make rooms   */

    /************************************************************************/
    /*                                                                      */
    /*    !   !  !!!   !!!!!  !!!!!   !!                                    */
    /*    !!  ! !   !    !    !       !!                                    */
    /*    ! ! ! !   !    !    !!!     !!                                    */
    /*    !  !! !   !    !    !                                             */
    /*    !   !  !!!     !    !!!!!   !!                                    */
    /*                                                                      */
    /* Citadel programs use readSysTab() and writeSysTab() to write an      */
    /* image of the external variables in RAM to disk, and later restore    */
    /* it.   The image is stored in ctdlTabl.sys .  If ctdlTabl.sys is lost,*/
    /* sysop.com will automatically reconstruct the hard way when invoked,  */
    /* and write a new ctdlTabl.sys out when finished.  CtdlTabl.sys is     */
    /* always destroyed after reading, to minimize the possibility of       */
    /* reading an out-of-date version.  In general, the technique works     */
    /* well and saves time and head-banging on bootup.  You should,         */
    /* however, note carefully the following cautions:                      */
    /*  o  "char firstExtern;" should >always< be the first                 */
    /*      declaration in citadel.h                                        */
    /*  o  "char lastExtern;"  shoule >always< be the last                  */
    /*      declaration in citadel.h                                        */
    /*  o  Whenever you change the declarations in 210Ctdl.h you should:    */
    /*     -->  destroy the current ctdlTabl.sys file                       */
    /*     -->  recompile and reload all citadel programs which access      */
    /*          ctdlTabl.sys -- currently citadel.com & consifurg.com       */
    /*     -->  use configur.com to build a new ctdlTabl.sys file           */
    /*                                                                      */
    /* If you ignore these warnings, little pixies will prick you in your   */
    /* sleep for the rest of your life.                                     */
    /************************************************************************/

    /************************************************************************/
    /* stuff to distinguish the various Citadel programs                    */
    /************************************************************************/
    /* #define TEST            1   */
#define CITADEL   0  /* prinipal program              */
#define CONFIGURE     1  /* unused                        */
#define NET       2  /* network downloader   (future) */
#define ARCHIVE   3  /* backup program (future)       */
    char  weAre;     /* set first thing by main()     */

    /************************************************************************/
    /*       Stuff to size system with:                                     */
    /************************************************************************/

    /* WARNING!!! if you expand MAXROOMS beyond 128, horrible, horrible     */
    /* things will happen to the userlog entries, which will no longer      */
    /* fit in the assigned 256-byte records and will overwrite things       */
    /* with normally undesirable results. 82Nov10CrT                        */
#ifdef TEST
#define MAXROOMS  6
#define MAXLOGTAB 4
#else

#define MAXROOMS       64  /* number of rooms allowed in system   */

#define MAXLOGTAB      180   /* number of log entries supported  */

#endif

#define SECTSIZE      512  /* 512 for MSDOS, 128 for CP/M */

#define SECSPERROOM   sizeof(ra.roomBuf)   /* (2 - CP/M)  sectors/room   */
#define ROOMSECTORS   (MAXROOMS*SECSPERROOM)
#define SECSPERLOG   sizeof(ra.logBuf)  /* (3-CP/M) sectors/log record  */
#define LOGSECTORS    (MAXLOGTAB*SECSPERLOG)

    /************************************************************************/
    /*    Stuff nowadays usually in bdscio.h:                               */
    /************************************************************************/

    /* values for functions to return: */
#define TRUE      1
#define FALSE     0
#define ERROR         -1

#define SAMESTRING   0  /* value for strcmp() & friend      */

    /* ASCII characters: */
/* #define NULL      0 */
#define SOH    1
#define STX    2
#define CNTRLa 1
#define CNTRLC    3
#define EOT    4
#define ACK    6
#define BELL      7
#define BACKSPACE 8
#define CNTRLI    9  /* aka tab           */
#define TAB    9  /* aka ^I            */
#define NEWLINE        '\n'  /* "linefeed" to philistines.    */
#define CNTRLl        12
#define CNTRLO        15
#define CNTRLp        16
#define XOFF          19   /* control-s            */
#define CNTRLt        20
#define NAK        21
#define CAN        24
#define CNTRLZ        26
#define CPMEOF    CNTRLZ
#define ESC        27   /* altmode           */
#define DEL      0x7F   /* delete char          */


    /* for packing & unpacking ints: */
#define BYTE         256
#define BDOS      5

    /************************************************************************/
    /*       Stuff for rooms:           */
    /************************************************************************/

#define LOBBY     0  /* Lobby> is >always< room 0.    */
#define MAILROOM  1  /* Mail>  is >always< room 1.    */
#define AIDEROOM  2  /* Aide> is >always< room 2.     */
#define LOGROOM      3  /* to record callers in       */

#define NAMESIZE       20  /* length of room names       */

#ifdef TEST
#define MSGSPERRM      58  /* should be >= MAILSLOTS     */
#else
#define MSGSPERRM      58  /* roombuf must be 256 bytes or less.  */
#endif

#define FILSPERRM      10  /* roombuf must be 256 bytes or less.  */
    char debug;       /* turns debug printout on and off  */
    char rtry;        /* guesses for hidden rooms      */
    int  roomfl;         /* file descriptor for ctdlRoom.sys */

#define INUSE     1  /* flags mask           */
#define PUBLIC    2  /* flags mask           */
#define CPMDIR    4  /* flags mask           */
#define PERMROOM  8  /* flags mask           */
#define ANON           16
#define BY_INVITE_ONLY 32
#define MSG_READ_ONLY 64
#define NO_UPLOADS 128
    struct roomTable {
        unsigned rtgen;     /* generation # of room       */
        unsigned rtflags;      /* public/private flag etc    */
        char     rtname[NAMESIZE];   /* name of room         */
        unsigned long rtlastMessage;   /* # of most recent message in room */
    }
    roomTab[MAXROOMS];

    struct roomBufer {
        unsigned rbgen;     /* generation # of room       */
        unsigned rbflags;      /* same bits as flags above      */
        char     rbname[NAMESIZE];   /* name of room         */
        char     rbdisk;    /* disk this rooms files are in 0=>none */
        char     rbuser[60];    /* user area for this rooms files   */
        struct {
            unsigned long rbmsgNo;   /* every message gets unique#    */
            unsigned int rbmsgLoc;  /* sector message starts in      */
        }
        msg[MSGSPERRM];
        /* is exactly 256 at present     */
    }
    roomBuf;        /* buffer current room is held in   */

    char thisRoom;       /* room currently in roomBuf  */

    /************************************************************************/
    /*             userlog stuff           */
    /************************************************************************/
#define CRYPTADD      117  /*             */

#define UCMASK    1  /* bit uppercase flag            */
#define LFMASK    2  /* bitmask for linefeed flag     */
#define EXPERT    4  /* bitmask for expert-user flag  */
#define TABMASK   8  /* bitmask for tab flag       */
#define AIDE      16 /* bitmask for aide flag      */
#define LOMASK    32 /* bitmask for show last old msg */
#define TWIT      64 /* auto new user?                       */
#define MORE     128 /* use 21 lines per screen and pause */

/* the following bits are set in lbnulls */
#define SYSOP    128 /* this guy is a Sysop */
#define NOPACE    64 /* do not pace the text output for typeout */
#define MAILSLOTS 58 /* twiddle to fit logBuf in 384 bytes  */

#define NULLMASK  1

#define GENSHIFT  3  /* lbgen >> GENSHIFT gives generation  */
#define CALLMASK  7  /* lbgen &  CALLMASK gives last visit  */
#define MAXVISIT  8  /* #visits we remember old newestLo for */
#define MAXGEN        32   /* six bits of generation => 64 of them */

    /* logbuf must be 384 bytes or less... including lbOverFlow, 384 or MORE*/
    struct logBuffer {
        unsigned  lbnulls;     /* nulls, twit, 6 empty */
        unsigned  lbflags;     /* UC, LF, EXPERT, TAB, AIDE, LO        */
        unsigned char lbwidth;     /* terminal width                */
        char      lbname[NAMESIZE]; /* caller's name         */
        char      lbpw[NAMESIZE]; /* caller's password       */
        char      lbgen[MAXROOMS];   /* 6 bits gen, two bits lastvisit   */
        unsigned  long  lbvisit[MAXVISIT];/* newestLo for this and 3 prev. visits */
        unsigned  lbslot[MAILSLOTS];/* for private mail         */
        unsigned  long  lbId[MAILSLOTS];   /* for private mail        */
        unsigned  char  BIO_waste[16];  /* 128 bits, 1 for each max room BIO */
        unsigned  char  pause_at;   /* how many lines to pause at */
        time_t    last_call;
#ifdef TEST
        char      lbOverFlow[235];
#else
        char      lbOverFlow[8];
#endif
    }
    logBuf;

    int thisLog;         /* entry currently in logBuf     */
    int thisSlot;        /* logTab slot logBuf was found via */

    int logfl;        /* log file fd          */

    struct {
        int  ltpwhash;      /* hash of password        */
        int  ltnmhash;      /* hash of name         */
        int  ltlogSlot;     /* location in userlog.buf    */
        char ltFlag;        /* for permanent and twit               */
        unsigned  long ltnewest;     /* last message on last call     */
    }
    logTab[MAXLOGTAB];
    int sizeLTentry;     /* contains size of a logTab entry  */

    char loggedIn;       /* global have-caller flag    */
    /*char scrn; */                   /* hold screen for fast modems          */

    char guess;       /* for pw            */

    /************************************************************************/
    /*       terminal stuff             */
    /************************************************************************/
#define SPECIAL        27  /* ESC   console escape char     */
    /*
    char *monthTab[13];
    char *dayTab[7];
    int  dayT;
    */
    int  date;
    int  month;
    int  year;
    int  hour;
    int  minute;
    int  lhour;                       /* logon time                         */
    int  lminute;
    /*
    char set; */                    /* for the soft clock  */
    char filter[128];    /* input character translation table   */
    char shave[8];       /* shave-and-a-haircut/2 bits pauses   */

    char prevChar;       /* for EOLN/EOParagraph stuff    */
    char expert;         /* true to suppress hints & noise   */
    char termWidth;      /* width                      */
    char termUpper;      /* uppercase-only flag        */
    char termLF;         /* LF-after-CR flag        */
    char termTab;        /* has-tabs flag        */
    char lasto;       /* show last old        */
    char aide;        /* aide-privileges flag       */
    char twit;                      /* twit? auto new user                  */
    char termNulls;      /* #nulls to send at eoln     */
    char termMore;
    char nopace;           /* do not pace output */
    char justLostCarrier;      /* room2.c / rmmodem.c  communication  */
    char crtColumn;      /* current position on screen    */

    char newCarrier;
    char onConsole;

    int msgfl;     /* message file fd--packed leaky circular queue */
    int callfl;    /* caller log fd */

    char sectBuf[SECTSIZE];    /* temporary buffer for random access  */

    char GMCCache;       /* to unGetMsgChar() into     */

    unsigned int oldChar;         /* old value of thisChar      */
    unsigned int oldSector;       /* old value of thisSector    */

    unsigned int thisChar;        /* next char in sectBuf       */
    unsigned int thisSector;      /* next sector in msgfl       */

    unsigned int catChar;      /* as in cat-and-mouse queue, or conCATenation. */
    unsigned int catSector;       /* where to write next message      */

    unsigned long oldestLo;  /* 32-bit ID# of first message in system*/
    unsigned long newestLo;  /* 32-bit ID# of last  message in system*/

    int    gloc;  /*for kludgey release of HELD msgs */

    unsigned int maxMSector;      /* # sectors (used) in ctdlmsg.sys  */
    char twited;                    /* a msg HELD from view except to aids  */
    char reply;       /* true if reply in mail      */
    /* values for showMess routine */
#define NEWoNLY   0
#define OLDaNDnEW 1
#define OLDoNLY   2
#define GLOBALnEW 3
#define LASTFIVE  4

    /************************************************************************/
    /*       modem stuff             */
    /************************************************************************/
    /* somestuff readfile() and sendfile() like: */
#define LOOPSPERSEC  5000
#define ERRORMAX       10
#define RETRYMAX       12
#define MINUTE         60
#define STARTUP         1
#define FINISH          2

char WCSecNum;
int WCChar;
char WCError;


    /* sendFile() globals because BDS C lacks coroutines and statics alike: */
    char SFcheckSum, SFeofSeen, SFRunning;
    unsigned int  SFi, SFthisSector, SFerrorCount, SFtries;
    int SFthisChar;
    char SFBuf[SECTSIZE];

    /* fiddle factor to timeout on no input: */
#define HITIMEOUT 2  /* count seems to be about 30 sec   */

#define NEWCARRIER   0x01  /* returned to main prog on login   */

#define MODEM     0  /* current user of system is     */
#define CONSOLE   1  /* one of these         */
    char modStat;        /* whether modem was on line     */
    /* LAST time it was checked      */

    char noChat;         /* TRUE to suppress chat attempts   */
    char whichIO;        /* CONSOLE or MODEM        */
    /*char visibleMode; */  /* make non-printables visible?  */
    char haveCarrier;    /* set if DCD == TRUE         */
    char exitToCpm;      /* system is being brought down  */
    long baud;        
    char textDownload;      /* read host files, TRUE => ASCII   */


    /*  output XON/XOFF etc flag... */
#define OUTOK     0  /* normal output        */
#define OUTPAUSE  1  /* a pause has been requested    */
#define OUTNEXT   2  /* quit this message, get the next  */
#define OUTSKIP   3  /* stop current process       */
#define OUTPARAGRAPH 4  /* skip to next paragraph     */
#define  IMPERVIOUS  5
    char outFlag;        /* will be one of the above      */

#define NEITHER   0  /* don't echo input        */
#define CALLER    1  /* echo to caller only --passwords etc */
#define BOTH      2  /* echo to caller and console both  */
#define ASTERISK  3  /* Echo Asterisks to both */

    char echo;        /* will be one of above three    */

    /* scratch externals for speed-critical code:   */
    char *fpc1, *fpc2, *fpc3;
    char   fc1,   fc2,   fc3;
    int    fi1,   fi2,   fi3;

    char usingWCprotocol;      /* true during Ward Christensen- */
    /* protocol file/message transfers. */
    /************************************************************************/
    /*    get/put message stuff               */
    /************************************************************************/
    char pullMessage;    /* true to pull current message  */
    unsigned  pulledMLoc;     /* loc of pulled message      */
    unsigned  long pulledMId;       /* id number of message to be pulled   */


    /************************************************************************/
    /* 1.5   buffered file I/O stuff             */
    /************************************************************************/
#define NSECTS    8  /*  8 sectors == 1024 byte buffer   */

#define BUFSZE    (NSECTS * SECTSIZE + 6)


    unsigned long freeSpace;    /* for readfile to decrament */

    unsigned long FDSectCount;     /* fileDirectory() keeps a count here  */
    char twitbit;           /* do we use the twitbit on new users */
    unsigned int  logsize;  /* number of users */
    unsigned int  nummsgs;  /* number of msgs per room */
    long          timeout;  /* sysop definable timeout */
    int           AT;       /* use AT BIOS call for Date/Time */
    int           time_allowed;  /* sysop definable allotment for user logon */


    char term_prg[80];    /* name of program/batch to run for terminal use */
    char msg_editor[80] ; /* name of program/batch to run for msg. creation */
    char sysopname[NAMESIZE]; /* NAME OF SYSOP */
    /************************************************************************/
    /*       message stuff              */
    /************************************************************************/
#define MAXTEXT      7000  /* maximum chars in edit buffer  */

    struct {
        char mbtext[MAXTEXT]  ;   /* buffer text is edited in      */

        unsigned int  mbheadChar    ;   /* start of message        */
        unsigned int  mbheadSector     ;   /* start of message        */

        char mbauth[ NAMESIZE];   /* name of author       */
        char mbdate[ NAMESIZE];   /* creation date        */
        char mbId[  NAMESIZE]; /* local number of message    */
        char mboname[NAMESIZE];   /* short human name for origin system  */
        char mborig[ NAMESIZE];   /* US 206 633 3282 style ID      */
        char mbsrcId[NAMESIZE];   /* message ID on system of origin   */
        char mbto[  NAMESIZE]; /* private message to         */
        char mbHeadline[60]; 
    }
    msgBuf;
    char hitEnter;
    struct text_info ti;
    int ANSI_text, ANSI_prompt, ANSI_header, ANSI_author, ANSI_bkgd;
    char ansi_dir[60], text_dir[60], main_dir[60], bat_dir[60], info_dir[60];
    struct record_53 call_record;
    char baud_asc[10];
    long callerno;
    char ansi_on;
    char doors;
    char init2[41];
    char off_hook[20];
    char on_hook[20];
    char dial_out[20];
    char attn[20];
    char answer_phone[20];
    char pause_at;
    char use_headline;
    char use_username;
    char grand_poobah;
    char lastExtern ;       /* this should ALWAYS be the last declaration  */
};


/***********************************************************************/
/*     defines to handle casesensitivity that BDS didn't care about    */
/***********************************************************************/

#define fClose fclose
#define fFlush fflush
#define fGetS  fgets
#define getstring getString
#define isAlpha isalpha
#define isSpace isspace
#define MakeMessage makeMessage
#define mprintf mPrintf
#define outmod outMod
#define putC putc
#define putCh putchar
#define putmsgChar putMsgChar
#define qSort qsort
#define sizeOf sizeof
#define slideltab slideLTab
#define sPrintf sprintf
#define strCmp strcmp
#define strLen strlen
#define toLower tolower
#define toUpper toupper
#define strCpy strcpy
#define RING 59997
#define MDM_OK 59998
#define MDM_ERR 59999
#define B300 300
#define B1200 1200
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400
#define B57600 57600
#define HISPEED 5
#define FAX 59009
#define USER 0
#define MSPEED 1
#define T_LOGON 2
#define WHICH_IO 3
#define S_MSGS 4
#define CURR_TIME 5
#define INIT_LINE 7
#define T_CHAT  6

/* special keys */
#define F1 0x3b
#define F2 0x3c
#define F3 0x3d
#define F4 0x3e
#define F5 0x3f
#define F6 0x40
#define F7 0x41
#define F8 0x42
#define F9 0x43
#define F10 0x44

#ifdef globl
struct ramarea ra;
char format[256];


/* File Stuff to Avoid U/L-D/L of DOS devices */
char *DOS_devices[]={"PRN","LPT1","LPT2","LPT3",
                     "AUX","COM1","COM2","COM3","COM4",
                     "CLOCK$","CLOCK\$","CON","NUL","CTDLTABL.SYS",
                     "CTDLLOG.SYS","CTDLROOM.SYS","CTDLMSG.SYS",
                     "CTDLCNFG.SYS","RESULTS.SYS", "CALLLOG.SYS",
                     "CALLNO.SYS","AUTOEXEC.BAT", "CONFIG.SYS",
                     "NEWFOLX.SYS","CCITCNFG.SYS",'\0'};

char *no_view[]={"COM","EXE","ZIP","ARC","ARJ","PAK","ZOO",
                 "LZH","SYS","Z","LZW","GIF","PCX","TIF",'\0'};

char *baud_tokens[]={"#MDM_OK","#MDM_ERR","#RING", "#B300", "#B1200", "#B2400",
                     "#B4800", "#B9600", "#HISPEED", "#FAX", '\0'};

int  baud_values[] = { 998, 999, 997, 0, 1, 2, 3, 4, 5, 9, 0 };

/*char
*baudstr[] = {"300", "1200", "2400", "4800", "9600", "19200", "38400"};*/
long baudstr[] = {(long) 300, 
                  (long) 1200, 
                  (long) 2400, 
                  (long) 4800, 
                  (long) 9600, 
                  (long) 19200, 
                  (long) 38400};


int upld;
int p_port;

char WCSecNum;
int WCChar;
char WCError;
char *version = "CopperCit 1.19J1";
char *babedate = "06Aug94";

struct baud_packet *baud_list;
struct xfer_protocol *up_menu;
struct xfer_protocol *down_menu;
long   timeout;
char pc[L_tmpnam];
char scrn;
int fix_hour;
int fix_month;
int fix_year;
int fix_date;
int fix_flag;
unsigned char bits[8] = {128, 64, 32, 16, 8, 4, 2, 1};
char first_flag;
char pause_override;
char no_cit_init;
char is_door;
char is_mailer;
FILE *callfl;  /* kludge for CTDL.C */
char sysopflag;
int init_modem_hourly;
int init_modem_done;
int save_minute;
char neverLoggedIn;
char did_read;
char ANSI_cls[5] = { ESC, '[','2','J','\0' } ;
char ANSI_reset[5] = { ESC,'[','0','m','\0'};
char byeflag;
char for_sysop;
long sysop_hold;
char clr_screen;
char no_stop;
char ansi_tut;
int blank_flag, bl_ok;

#else

extern struct ramarea ra;
extern char format[256];
extern char *DOS_devices[], *baud_tokens[];
extern int  upld;
extern int p_port;

extern char WCSecNum;
extern int WCChar;
extern char WCError;
extern char *version, *babedate;
extern struct baud_packet  *baud_list;
extern int baud_values[9];
/* extern struct record_53 call_record; */
extern struct xfer_protocol *up_menu;
extern struct xfer_protocol *down_menu;
extern long timeout;
extern char pc[L_tmpnam];
extern char scrn;
/* extern char baud_asc[10]; */
extern int fix_hour;
extern int fix_month;
extern int fix_year;
extern int fix_date;
extern int fix_flag;
extern unsigned char bits[8];
extern char first_flag;
extern char pause_override;
/* extern long callerno; */
extern char no_cit_init;
extern char is_door;
extern char is_mailer;
extern char sysopflag;
extern int init_modem_hourly;
extern int init_modem_done;
extern int save_minute;
extern char neverLoggedIn;
extern char did_read;
/* extern char ansi_on; */
extern char ANSI_cls[5];
extern char ANSI_reset[5];
extern long baudstr[];
extern char byeflag;
extern char for_sysop;
extern long sysop_hold;
extern char clr_screen;
extern char no_stop;
extern char ansi_tut;
extern int blank_flag, bl_ok;
#endif

