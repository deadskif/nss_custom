#include <stdio.h>
#include <pwd.h>
#include <nss.h>

#include "config.h"
#define BUFLEN 2048
int main (int argc, char const* argv[])
{
    struct passwd pw;
    char buf[BUFLEN];
    int errnop;
    enum nss_status ret;

    PREFIX_DEFINED(setpwent)(0);
    while (1) {
        ret = PREFIX_DEFINED(getpwent_r)(&pw, buf, BUFLEN, &errnop);
        if (ret != NSS_STATUS_SUCCESS)
            break;
        printf("%s (%d)\tHOME %s\tSHELL %s\n", pw.pw_name,
            pw.pw_uid, pw.pw_dir, pw.pw_shell);
    }

    PREFIX_DEFINED(endpwent)(0);
    
    return 0;
}

