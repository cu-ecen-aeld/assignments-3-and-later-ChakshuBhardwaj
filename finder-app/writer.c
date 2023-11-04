#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>

int main(int argc , char *argv[])
{
	FILE *file = NULL;
	char *fileaddress = NULL;
	char *filedata = NULL;
	char *filepermissions = "wr";
	int ArgumentsExpected = 2;
	openlog(NULL,0,LOG_USER);
	if(argc != 3)
	{
		syslog(LOG_ERR,"Invalid number of arguments, Expected %d but entered %d\n", ArgumentsExpected, (argc -1));
		exit(1);
	}

	else
	{
		fileaddress = argv[1];
		filedata = argv[2];
		
		file = fopen(fileaddress,filepermissions);
		if (file == NULL)
		{
			syslog(LOG_ERR,"Unable to open the file!!\n");
			exit(1);
		}

		fprintf(file,"%s",filedata);
		fclose(file);

		syslog(LOG_DEBUG,"Writing %s to %s",filedata,fileaddress);
		closelog();
		
	}	
	exit(0);
}
