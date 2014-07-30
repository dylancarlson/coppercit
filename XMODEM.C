
#include "ctdl.h"
#define CRCTYPE 2
char crcmode;
int SSIZE;
int WCTrueCount, FDSectCount;;
char *WCBuf;

#define ABORT 2
char CarrDet();
/************************************************************************/
/*      readFile() accepts a file from modem using Ward Christensen's   */
/*      protocol.  (ie, compatable with xModem, modem7, yam, modem2...) */
/*      Returns:        TRUE on successful transfer, else FALSE         */
/************************************************************************/
char xreadFile(upBuf)
          /* pc will accept the file one character at a time. */
int   upBuf;     /* returns ERROR on any problem, & closes the file  */
          /* when handed ERROR as an argument.                */
{
    char    badSector, writeOk, *nextChar,
       acceptable[5], goodCRC, charIn, mbtext[2064];
    unsigned totalCRC;
    int     i, firstchar, lastSector, thisSector, thisComplement, tries,
       SIZE, toterr, checksum, sectcount; /* , sectfree; */

    if (!getYesNo("Ready For Xmodem-CRC/CSum"))
      return(FALSE);

    sectcount   =
    lastSector  =
    tries       =
    toterr      = 0;

    writeOk     =
    charIn      = TRUE;
    acceptable[0] = EOT;
    acceptable[1] = SOH;
    acceptable[2] = STX;
    acceptable[3] = CAN;
    acceptable[4] = 0;


/*    printf("%s Block #0 (Try=0, Errs=0)\n", (goodCRC) ? "CRC" : "CHK");
*/
    do {

       badSector = FALSE;

   /* get synchronized: */
   do {
       if (charIn) {
      
          if (!(goodCRC = WCstartUp('C', 7, &firstchar, acceptable))) {
       
             if (!WCstartUp(NAK, 7, &firstchar, acceptable)) {
                cprintf("Startup NAK\n\r");
                return FALSE;
             }
          }
      
          cprintf("%s\n\r", (goodCRC) ? "CRC" : "CHK");
          charIn = FALSE;
       }
       else
          firstchar = receive(10);
       
       if (!(carrDet()) || firstchar==CAN)
       badSector = ABORT;
   } while (firstchar !=STX && firstchar != SOH && firstchar != CAN &&
       firstchar != EOT && firstchar != ERROR);

   
   if (firstchar == ERROR && badSector != ABORT)
       badSector = TRUE;

   if (firstchar == SOH || firstchar == STX) {   /* found StartOfHeader -- 
                                                    read sector# in */
   
      if (firstchar==STX)
          SIZE = 1024;
      else
          SIZE = 128;
   
      thisSector = receive(1);
      thisComplement = receive(1);     /* 1's comp of thisSector */
      if ((thisSector + thisComplement) != 0xFF){
         cprintf("Bad Sector != 255");
         badSector = TRUE;}
      else {
      
         if (thisSector == (lastSector + 1) % 256) {
          /* right sector... let's read it in */
            checksum    = 0;
            nextChar    = mbtext;
          
            for (i=SIZE;  i;  i--) {
               *nextChar = receive(2);
               checksum = (checksum + *nextChar++) & 0xFF;
            }
            totalCRC = receive(2);
          
            if (goodCRC) {
               
               if (totalCRC != 0xFFFF) {
                  
                  if ((checksum = receive(1)) != ERROR) {
                     totalCRC = (totalCRC << 8) + checksum;
            
                     if ((calcrc(mbtext,SIZE)) != totalCRC)
                        badSector=TRUE;
                  }
               }
          }
          else {
              
              if (checksum != totalCRC)
                 badSector = TRUE;
          }

          if (!badSector) {
             tries = 0;
             lastSector = thisSector % 256;
             ++sectcount;
             cprintf("Block #%d (Try=0, Errs=%d)\r",sectcount,toterr);
         
             if (tries && toterr)
                cprintf("\r\n");

             /* write sector to where-ever: */
             nextChar = mbtext;
             
/*             for (i=SIZE, writeOk=TRUE; i && writeOk; i--) {
           writeOk = putFLChar(*nextChar++, upBuf);
                 writeOk = (fwrite(nextChar++,1,1,upBuf) == 1);
             }
*/
            writeOk = (_write(upBuf, nextChar, SIZE)) == SIZE;
            
            if (writeOk) {
               outMod(ACK);
            }

          }
      }
      else {
          /* not expected sector... */
          if (thisSector != lastSector)
             badSector = TRUE;
          else {
         /* aha -- sender missed an ACK and resent last: */
               for (i=SIZE;  i;  i--) {
                   receive(2);     /* Eat Garbage */
               }
         outMod(ACK);    /* back in synch! */
          }
      }
    }   /* end of "if (thisSector + thisComplement == 255"      */
   }       /* end of "if (firstChar == SOH)"                       */

   if (badSector==TRUE)  {
       tries++;
       if (lastSector != 0)
       toterr++;

       while (receive(2) != ERROR)
      ;
       cprintf("Block #%d (Try=%d, Errs=%d)  \r",sectcount,tries,toterr);
       if (tries && toterr)
          cprintf("\r\n");
       outMod(NAK);
   }
}  while ( firstchar != EOT && tries < ERRORMAX && writeOk &&
        badSector != ABORT);

    if (!writeOk) {
       outMod(CAN);
       iChar();

/*        switch(i=errno()) {
       case 2:
      mPrintf("\n Dir full!\n ");
      break;
       default:
      mPrintf("\n %s\n ", errmsg(i));
   }          
*/
      mPrintf("Disk Error\n ");
      
      return(FALSE);
    }

    if (firstchar != EOT || tries >= ERRORMAX) {
       outMod(CAN);
       return(FALSE);
    }
    else {
       outMod(ACK);
       return ((sectcount == 0) ? FALSE : TRUE);
    }
}

/*
 * This function calculates the CRC used by the XMODEM/CRC Protocol
 * The first argument is a pointer to the message block.
 * The second argument is the number of bytes in the message block.
 * The function returns an integer which contains the CRC.
 * The low order 16 bits are the coefficients of the CRC.
 * Taken from XYMODEM.DOC -- HAW.
 */
calcrc(ptr, count)
char *ptr;
int count;
{
    int i;
    unsigned crc;

    crc = 0;
    while (--count >= 0) {
      crc = crc ^ *ptr++ << 8;
      for (i = 0; i < 8; ++i)
          if (crc & 0x8000)
         crc = crc << 1 ^ 0x1021;
          else
         crc = crc << 1;
      }
    return (crc & 0xFFFF);
}

/************************************************************************/
/*      WCstartUp() Starts up WC and (sometime) YMODEM                  */
/************************************************************************/
WCstartUp(startChar, tries, charIn, lookFor)
char startChar;
int  tries, *charIn;
char *lookFor;
{
    int retVal;
  /*  char *strchr();*/

    for (; tries && carrDet(); tries--) {
   outMod(startChar);
   retVal = receive(10);
   if (retVal != ERROR) {
       *charIn = retVal;
       if (strchr(lookFor, *charIn) != NULL) {
      return TRUE;
       }
   }
    }
    return FALSE;
}

/************************************************************************/
/*      xmoddown() reads a file from current directory using WC protocol*/
/************************************************************************/
/* xmoddown(filename,size, mode) */
xsendfile(filename,size/*, mode*/)
 char *filename/*, mode*/;
 int  size;
{
    char        mAbort(), fname[NAMESIZE], tWCbuf[1024];
    int c;
    unsigned    totsect, time, min, sec;
    int fbuf;

    WCBuf = tWCbuf;

 /*   outFlag = OUTOK; */
    SSIZE = size;
    ra.usingWCprotocol = FALSE; /* have to do it that way since oChar() won't  */
              /* print anything if usingWCprotocol == TRUE.  */
/*    if (mode == 1) {
   readMark();
   return;
    }

    if (mode == 2) {
   xmodshowMsg(filename[0], filename[1]);
   return;
    } */    /* later for msgs */


 
    unspace(filename, fname);
 
    if (upld == NULL)
       setSpace(ra.roomBuf.rbdisk, ra.roomBuf.rbuser);

    if ((fbuf = open(fname, O_RDONLY | O_BINARY)) == -1) {
      /* if (noStop != TUTORIAL)  */
       sprintf(format,"\n No '%s'\n ", fname);
       mPrintf (format);
   setSpace(ra.homeDisk, ra.homeUser);
   return(ERROR);
    }
   doCR();

    mPrintf("download ready.");
    doCR();
 

    ra.usingWCprotocol = TRUE;
    if (!xmodWC(STARTUP)) {
   close(fbuf);
   setSpace(ra.homeDisk, ra.homeUser);
   return(ERROR);
    }
    cprintf("%s\n\r", (crcmode) ? "CRC" : "Checksum");

    while (((read(fbuf, &c, 1)) == 1 /*ERROR*/ )&&((carrDet()) || ra.onConsole)
      && !WCError) {
   xmodChar( (char) c);
    }

    if (ra.usingWCprotocol)
   xmodWC(FINISH);

    close(fbuf);
 
    setSpace(ra.homeDisk, ra.homeUser);
    return(TRUE);
}

/************************************************************************/
/*      xmodChar() sends data via WC protocol.                        */
/************************************************************************/
xmodChar(c)
 char   c;
{
    int       ck;
    unsigned  crcv;
    int       i, j, m;

    if (WCError)
        return(FALSE);

    WCBuf[WCChar++] = c;                /* Store in buffer              */
    if (WCChar != SSIZE)
        return(TRUE);


    for (i = 0; i < ERRORMAX; i++) {    /* Time to transmit             */
        if (i)
            cprintf("\r\n");
        cprintf("Block #%3d (Try=%d)\r", WCTrueCount, i);
        outMod((SSIZE==128) ? SOH : STX);
   outMod(WCSecNum);
        outMod(~WCSecNum);
        for (j = ck = 0; j < SSIZE; j++) {
            outMod(WCBuf[j]);
            ck += WCBuf[j];
        }
        if (crcmode) {
            crcv = calcrc(WCBuf, SSIZE);
            outMod(((crcv & 0xFF00) >> 8));
/*            while (interpret(pMIReady)) inp();  */
            outMod(crcv & 0x00FF);
        }
        else
            outMod(ck);
        m = receive(MINUTE);
        if (m == ACK || m == CAN || !(carrDet()))
            break;
    }
    WCChar = 0;
    WCSecNum++;
    WCTrueCount++;
    if (m == ACK)
        return(TRUE);

    /* else */
    WCError = TRUE;
    return(FALSE);          /* Indicates receiver is allergic to us */
}

/************************************************************************/
/*      doWC() is the initialization routine for WC downloading.  If    */
/*      called with mode == STARTUP, it sets up globals and gets ready  */
/*      for the initial NAK.  If mode == anything else, then it cleans  */
/*      up after the downloader.                                        */
/************************************************************************/
xmodWC(mode)
 char   mode;              /* See above */
{
    int i, m;

    if (mode == STARTUP) {      /* Setup globals for the coming fun.    */
   WCError  = FALSE;
   WCSecNum = WCTrueCount = 1;
   i = WCChar = crcmode = 0;
        while (TRUE) {                  /* Get that darn initial NAK    */
            m = receive(MINUTE);        /* Seems to be about 30 seconds */
            if (m == CAN || m == ERROR) /* Didn't get initial           */
                return(FALSE);          /* NAK, so time out ..          */
            if (m == NAK || m == 'C') { /* There it is!               */
                if (m == 'C')
                    crcmode = CRCTYPE;
                else
                    crcmode = FALSE;
                return TRUE;
            }
            if (++i == ERRORMAX)        /* 10 chars, no NAKs?           */
                return(FALSE);          /* Then ferget it.              */
        }
    }
    else {                      /* Cleanup after downloader */
        if (WCError)
       return;
        while (WCChar != 0 && xmodChar(' '))  /* Do final sector */
            ;
        if (WCError)
       return;
        for (i = 0; i < ERRORMAX; i++) {
            outMod(EOT);
       if (receive(MINUTE) == ACK || !ra.haveCarrier)
                break;
        }
    }
}

/************************************************************************/
/* 88Jan30 ASG  Allowed room name of origin to be written to disk       */
/* 87May10 LSR  Added msgput variable to return a TRUE if putmessage()  */
/*              called.  Completely rewrote getStr() to properly use    */
/*              lim, & to test & quit on ERROR detection.               */
/* 87Apr26 JE   Added routine to convert all CRs to NEWLINES.           */
/************************************************************************/
#if 0
/************************************************************************/
/*      insrtMsg() reads "temp.msg" and inserts uploaded msgs into the  */
/*      message base.                                                   */
/************************************************************************/
insrtMsg()
{
    int         c, ch, i, msgCount, msgput;
    char        putMessage();
    FILE        tempfl;
    msgCount=0;
    msgput=FALSE;

    if (tempfl = fopen("temp.msg", tempfl) == NULL) {
        mPrintf("\n can't open TEMP.MSG\n ");
        return(msgput);
    }

    do {

        /* Clear msgBuf out */
        ra.msgBuf.mbauth[0]  =
        ra.msgBuf.mbdate[0]  =
        ra.msgBuf.mborig[0]  =
        ra.msgBuf.mboname[0] =
        ra.msgBuf.mbroom[0]  =
        ra.msgBuf.mbsrcId[0] =
        ra.msgBuf.mbtext[0]  =
        ra.msgBuf.mbto[0]    =
        ra.msgBuf.mbmisc[0]  = '\0';

        do {            /* find start of msg */
            c = getc(tempfl);
        } while ((c != 0xFF) && (c != ERROR));

        do {
            switch (c = getc(tempfl)) {
                case ERROR:
                    fclose(tempfl);
                    unlink("temp.msg");
                    return(msgput);
                case 'A':
                    getStr(msgBuf.mbauth,  NAMESIZE, tempfl);
                    break;
                case 'D':
                    getStr(msgBuf.mbdate,  NAMESIZE, tempfl);
                    break;
                case 'M':
                    getStr(msgBuf.mbtext,  MAXTEXT-1,  tempfl);
                    break;
                case 'N':
                    getStr(msgBuf.mboname, NAMESIZE, tempfl);
                    if (strCmpU(msgBuf.mboname, nodeName) == SAMESTRING)
                        msgBuf.mboname[0] = '\0';
                    break;
                case 'O':
                    getStr(msgBuf.mborig,  NAMESIZE, tempfl);
                    break;
/*              case 'R':
                    getStr(msgBuf.mbroom,  NAMESIZE, tempfl);
*/                  break;
                case 'T':
                    getStr(msgBuf.mbto,    NAMESIZE, tempfl);
                    break;
/*              case 'X':
                    getStr(msgBuf.mbmisc,  NAMESIZE, tempfl);
*/                  break;
                default: /* dump unknown field */
                    while ((ch = getc(tempfl)) && ch != ERROR)
                        ;
/*                  getStr(msgBuf.mbtext, MAXTEXT, tempfl);
                    msgBuf.mbtext[0] = '\0';
                    break;  */
            }
        } while ((c != 'M') && (isAlpha(c)));

        for (i=0; msgBuf.mbtext[i]; i++) {      /* Convert all \r to \n */
            if (msgBuf.mbtext[i] == '\r')
            msgBuf.mbtext[i] = NEWLINE;
        }

        if (c=='M' && putMessage(/* uploading = */ TRUE)) {
/*            noteMessage(0, ERROR);
            mPrintf("\n Inserted msg #%d", ++msgCount); */
            msgput=TRUE;
        }
    } while (c != ERROR);       /* If c==ERROR, returned in the switch() */

    fclose(tempfl);
    unlink("temp.msg");

}

#endif
/************************************************************************/
/*      getStr() reads in one line from fp and returns it in dest.      */
/************************************************************************/
getStr(dest, lim, fp)
 char   *dest;
 FILE *fp;
 int    lim;
{
    char c;

    while (lim && (c = getc(fp)) && c != 0xFF) {
        lim--;
        *dest++ = c;
    }
    *dest = '\0';
}
