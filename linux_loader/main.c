/*
 * Copyright 2010 Mayur Pawashe
 * https://zgcoder.net
 
 * This file is part of skycheckers.
 * skycheckers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * skycheckers is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
 */

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

