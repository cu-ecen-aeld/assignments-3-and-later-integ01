#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char* argv[])
{
   int num;
   FILE *fptr;

   openlog("assignment2Log", LOG_USER,LOG_DAEMON);
		
   if (argc < 3)
   {
	printf("Error: No FILE DIR or SEARCH string parameters specified\n");
	syslog(LOG_ERR, "Error: No FILE DIR or SEARCH string parameters specified");
	exit(1);
   }

   char *writeFileName = argv[1];
   char *writeStr = argv[2];


   // use appropriate location if you are using MacOS or Linux
   fptr = fopen(writeFileName,"w");
   if(fptr == NULL)
   {
      printf("Error!");   
      syslog(LOG_ERR, "Error: Open file to write failed");
      exit(1);             
   }
   syslog(LOG_DEBUG, "Writting %s to %s",writeStr, writeFileName);

   fprintf(fptr,"%s",writeStr);
   fclose(fptr);

   return 0;
}
