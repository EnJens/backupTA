/*
 *   This file is part of BackupTA.

 *  BackupTA is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  BackupTA is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with BackupTA.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdint.h>
#include <android/log.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <cutils/sockets.h>
#define TAG "DumpTA"
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__); }

void dumphex(char *data, int offset, int size, int blocklength);

int main(int argc, char **argv)
{
  struct sockaddr_in serv_addr;
  LOGV("Starting up...");
  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s /path/to/TA/blockdevice", argv[0]);
    return -1;
  }
  LOGV("Using file %s\n", argv[1]);
  FILE *ta = fopen(argv[1], "rb");
  if(!ta)
  {
    LOGV("Error opening %s: %s\n", argv[1], strerror(errno));
    return -1;
  }
  fclose(ta);

  int s = android_get_control_socket("tad");
  if(s < 0)
  {
    LOGV("Error creating socket: %s\n", strerror(errno));
    return -1;
  }

  listen(s, 5);

  while(1)
  {
    char buffer[1024];
    int offset=0;
    int sz = 0;
    int clifd = accept(s, NULL, NULL);
    if(clifd < 0)
    {
      LOGV("Error accepting client!?\n");
      continue;
    }

    LOGV("Accepted new client on fd %d\n", clifd);
    if((sz = read(clifd, buffer, 4)) != 4 || memcmp(buffer, "\42\42\42\42", 4) != 0)
    {
      close(clifd);
      continue;
    }

    // Reply
    write(clifd, "\43\43\43\43", 4);

    LOGV("Got expected client, w00t, w00t\n");
    ta = fopen(argv[1], "rb");
    while((sz = fread(buffer, 1, 1024, ta)) > 0)
    {
      write(clifd, buffer, sz);
      offset += sz;
    }
    fclose(ta);
    LOGV("Done dumping TA, %d bytes\n", offset);
    close(clifd);
  }
  return 0;
}

