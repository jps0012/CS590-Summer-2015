#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
int main()
{
uid_t uid;
gid_t gid;
struct passwd *mypasswd;
char *path, *home;
char hostname[100];
uid = getuid();
gid = getgid();
mypasswd = getpwuid(uid);
gethostname(hostname,sizeof(hostname));
printf("---------------------------\n");
printf("System name: %s\n", hostname);
printf("User information:\n");
printf("full name: %s\n", mypasswd->pw_gecos);
printf("log name: %s\n", mypasswd->pw_name);
printf("uid: %d\n", mypasswd->pw_uid);
printf("gid: %d\n", mypasswd->pw_gid);
printf("HOME dir: %s\n", mypasswd->pw_dir);
printf("Default shell: %s\n",mypasswd->pw_shell);
printf("-------------------------------\n");
system("date");
return 0;
}
