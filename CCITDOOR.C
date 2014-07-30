/****************************************************************

ccitdoor.c is the mini-module used to call doors and return to
           CopperCit
           
****************************************************************/

#include <dir.h>
#include <process.h>
#include <stdio.h>

main(int argc, char *argv[])

{

 char remember[128];
 
 getcwd(remember, 128);

 system("dropfil.bat");

 chdir(remember);
 
 execlp("ctdl.exe", "ctdl.exe", "N", ( argc > 1 ? argv[1] : NULL), NULL);
 
}
             