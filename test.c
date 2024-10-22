#include <linux/version.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

int main(int argc, char **argv) {
  struct utsname buf;
  if (uname(&buf) < 0) {
    perror("Error calling uname()");
    return 1;
  }

  printf("utsname.release: %s\n", buf.release);

  printf("未转换直接的数据显示=\n");
  printf("LINUX_VERSION_CODE: %d\n", LINUX_VERSION_CODE);
  printf("buf.machine = %s, buf.nodename = %s, buf.release = %s, buf.sysname = %s, buf.version = %s, buf.__domainname = %s\n", buf.machine, buf.nodename, buf.release, buf.sysname, buf.version, buf.__domainname);
  printf("转换后的数据显示=\n");

  int version = atoi(buf.release);
  char* firstDot = strstr(buf.release, ".");
  int patchlevel = atoi(firstDot + 1);
  char* secondDot = strstr(firstDot + 1, ".");
  int sublevel = atoi(secondDot + 1);

  printf("LINUX_VERSION_CODE: %d\n", LINUX_VERSION_CODE);
  printf("KERNEL_VERSION(%d, %d, %d): %d\n", version, patchlevel, sublevel,
         KERNEL_VERSION(version, patchlevel, sublevel));

  return 0;
}