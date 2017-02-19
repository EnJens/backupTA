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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/capability.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
#include <selinux/android.h>
#include <selinux/label.h>
#include <selinux/avc.h>

#ifdef DEBUG
#include <errno.h>
#define DEBUGPRINT(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define DEBUGPRINT(...)
#endif

int main(int __unused argc, char __unused **argv)
{
  struct __user_cap_header_struct capheader;
  struct __user_cap_data_struct capdata[2];

  memset(&capheader, 0, sizeof(capheader));
  memset(&capdata, 0, sizeof(capdata));
  capheader.version = _LINUX_CAPABILITY_VERSION_3;
  capdata[CAP_TO_INDEX(CAP_SETUID)].effective |= CAP_TO_MASK(CAP_SETUID);
  capdata[CAP_TO_INDEX(CAP_SETGID)].effective |= CAP_TO_MASK(CAP_SETGID);
  capdata[CAP_TO_INDEX(CAP_SETUID)].permitted |= CAP_TO_MASK(CAP_SETUID);
  capdata[CAP_TO_INDEX(CAP_SETGID)].permitted |= CAP_TO_MASK(CAP_SETGID);
  if (capset(&capheader, &capdata[0]) < 0) {
    DEBUGPRINT("Could not set capabilities: %s\n", strerror(errno));
    return -1;
  }
  uint32_t user = 0;
  uint32_t group = 0;
  gid_t extra_groups[] = {0, 2000, 1015, 1028, 2993, 3003};

  if(setgroups(6, extra_groups))
  {
    DEBUGPRINT("Error setting supplementary groups\n");
    return -1;
  }

  if(setresgid(group, group, group) || setresuid(user, user, user)) {
    DEBUGPRINT("setresgid/setresuid failed\n");
    return -2;
  }

  char *orig_ctx_str = NULL;
  char *ctx_str = NULL;
  context_t ctx = NULL;
  int rc = -1;

  rc = getcon(&ctx_str);
  if(rc)
  {
    DEBUGPRINT("Error getting current selinux context: %s\n", strerror(errno));
    return -1;
  }
  ctx = context_new(ctx_str);
  orig_ctx_str = ctx_str;
  if(!ctx)
  {
    DEBUGPRINT("Error creating new context from ctx_str: %s\n", strerror(errno));
    return -1;
  }
  rc = context_type_set(ctx, "platform_app");
  if(rc)
  {
    DEBUGPRINT("Error setting context type: %s\n", strerror(errno));
    return -2;
  }
  ctx_str = context_str(ctx);
  rc = setcon(ctx_str);
  if(rc)
  {
    DEBUGPRINT("Error setting context: %s\n", strerror(errno));
    return -3;
  }

  DEBUGPRINT("Executing secondary payload\n");
  int ret = execlp("/system/bin/screenrecord", "screenrecord", NULL);
  DEBUGPRINT("Error executing file: %s\n", strerror(errno));
  return 0;
}

