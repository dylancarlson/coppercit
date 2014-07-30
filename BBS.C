#include <process.h>
#include <stdio.h>

void main(int argc, char *argv[]) 

{
 FILE *fp;
 int exit_value=0;
 char cmd_line[128];

 exit_value = spawnlp( P_WAIT, "ctdl.exe", &argv[1], &argv[2], &argv[3],
		       &argv[4], &argv[5], &argv[6], &argv[7], &argv[8],
		       &argv[9], &argv[10], &argv[11], &argv[12], &argv[13],
		       &argv[14], &argv[15], &argv[16], &argv[17], &argv[18],
		       &argv[19], &argv[20], NULL);

 while(exit_value > 0 && exit_value < 100) {
      if ((fp = fopen ( "RUNBAT.TXT", "rt")) != NULL) {
	 fgets( cmd_line, 128, fp);
	 fclose(fp);
	 system(cmd_line);
      }
      exit_value = spawnlp(P_WAIT, "ctdl.exe", "N", NULL);
 }
}
      