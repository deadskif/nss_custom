/*
 * Copyright 2014 Vladimir Badaev.
 *
 * This file is part of NSS-Custom.
 *
 * NSS-Custom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * NSS-Custom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with NSS-Custom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <pwd.h>
#include <nss.h>

#include "config.h"
#define BUFLEN 2048
int main (int argc, char const* argv[])
{
    struct passwd pw;
    int errnop;
    char buf[BUFLEN];
    enum nss_status ret;

    syslog(LOG_INFO, "test pwent");
    PREFIX_DEFINED(setpwent)(0);
    while (1) {
        ret = PREFIX_DEFINED(getpwent_r)(&pw, buf, BUFLEN, &errnop);
        if (ret != NSS_STATUS_SUCCESS)
            break;
        printf("%s (%d)\tHOME %s\tSHELL %s\n", pw.pw_name,
            pw.pw_uid, pw.pw_dir, pw.pw_shell);
    }

    PREFIX_DEFINED(endpwent)();
    
    syslog(LOG_INFO, "test pwnam");
    ret = PREFIX_DEFINED(getpwnam_r)("rooat", &pw, buf, BUFLEN, &errnop);
    if (ret == NSS_STATUS_SUCCESS) {
        printf("%s (%d)\tHOME %s\tSHELL %s\n", pw.pw_name,
            pw.pw_uid, pw.pw_dir, pw.pw_shell);
    } else {
        printf("Not Found\n");
    }
    
    return 0;
}

