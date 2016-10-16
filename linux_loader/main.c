#include <stdio.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <string.h>

/*
 * Note: this function is licensed under the GPL v3 or later and is derived from:
 * http://apps.linuxaudio.org/apps/all/edrummer
 * Note 2: This code is probably unnecessary; if I was more bright back then,
 * I would have only just used/altered the run.sh script and make it executable
 * eDrummer for Linux
 * Copyright 2013 Jeff Glatt
 */

/******************* get_exe_name() ******************
 * Gets the path name where this exe is installed
 *
 * buffer = Where to format the path. Must be at least
 *                 PATH_MAX in size.
 */
static void get_exe_name(char* buffer)
{
  char linkname[64];
  pid_t pid;
  unsigned long offset;

  pid = getpid();
  snprintf(&linkname[0], sizeof(linkname), "/proc/%i/exe", pid);
  if (readlink(&linkname[0], buffer, PATH_MAX) == -1)
    offset = 0;
  else
  {
      offset = strlen(buffer);
      while (offset && buffer[offset - 1] != '/') --offset;
      if (offset && buffer[offset - 1] == '/') --offset;

  }
  buffer[offset] = 0;
}

int main(int argc, char *argv[])
{
	char runPath[PATH_MAX];
	get_exe_name(runPath);
	strcat(runPath, "/Data/run.sh");

	char executionString[PATH_MAX + 256];
	sprintf(executionString, "bash %s", runPath);
	system(executionString);
	
	return 0;
}

