/*********************************************************
* (C) ZE CMS, Humboldt-Universitaet zu Berlin 
* Written 2010 by Daniel Rohde <d.rohde@cms.hu-berlin.de>
*             and Daniel Stoye <stoyedan@cms.hu-berlin.de>
**********************************************************/
/*** CHANGES:
  2011-31-03:
      - fixed minor direct call bug reported by Tony H. Wijnhard <Tony.Wijnhard@mymojo.nl>
  2010-22-11:
      - fixed effective groups bug reported by Hanz Makmur <makmur@cs.rugers.edu>
*/
/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>

#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pwd.h>
#include <grp.h>


int main(int argc, char *argv[])
{
	struct passwd *pw = NULL;

	char *remote_user = getenv("WEBDAV_USER");
	if (remote_user == NULL) remote_user = getenv("REDIRECT_WEBDAV_USER");
	if (remote_user == NULL) remote_user = getenv("REMOTE_USER");
	if (remote_user == NULL) remote_user = getenv("REDIRECT_REMOTE_USER");

	if (remote_user != NULL) {
		int pos = strstr(remote_user, "@EXAMPLE.ORG") != NULL ? strcspn(remote_user, "@") : 0;
		if (pos > 0) remote_user[pos]='\0';
		pw = getpwnam(remote_user);
	}

	if ((pw != NULL)  && ( pw->pw_uid != 0)) {
		char *krbticket = getenv("KRB5CCNAME");
		/* copy ticket file: */
		if (krbticket != NULL) {

			strtok(krbticket, ":");
			char *srcfilename = strtok(NULL, ":");

			char dstfilename[1000];
			snprintf(dstfilename,1000,"/tmp/krb5cc_webdavcgi_%s", pw->pw_name);

			FILE *src, *dst;
			if ((src = fopen(srcfilename, "rb")) !=NULL && (dst = fopen(dstfilename, "wb")) != NULL ) {
				char c;
				while (!feof(src)) {
					c = fgetc(src);
					if (!ferror(src) && !feof(src)) fputc(c, dst);
				}
				fclose(src);
				fclose(dst);

				/* user needs access rights: */
				chown(dstfilename, pw->pw_uid, pw->pw_gid);

				/* put copy into the environment: */
				char krbenv[1000];
				snprintf(krbenv,1000,"KRB5CCNAME=FILE:%s",dstfilename);
				putenv(krbenv);
			}
		}
		if (initgroups(pw->pw_name,pw->pw_gid)==0 && setgid(pw->pw_gid)==0 && setuid(pw->pw_uid)==0) execv("webdav.pl",argv);
		else printf("Status: 500 Internal Sever Error");
	} else {
		printf("Status: 404 Not Found\r\n");
		printf("Content-Type: text/plain\r\n\r\n");
		printf("404 Not Found - your wrapper\n");
		printf("remote_user: %s\n",remote_user);

	}
}
