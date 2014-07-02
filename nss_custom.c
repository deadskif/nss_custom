/*
 * Copyright 2014 Vladimir Badaev.
 *
 * This file is part of NSS-Custom.
 *
 * NSS-Custom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * NSS-Custom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with NSS-Custom.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <pwd.h>
#include <nss.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"



#define COPY_IF_ROOM(s) \
  ({ size_t len_ = strlen (s) + 1;              \
     char *start_ = cp;                         \
     buflen - (cp - buffer) < len_              \
     ? NULL                                     \
     : (cp = mempcpy (cp, s, len_), start_); })

#define PWD_DATA_LIST
static int pwd_data_inited = 0;
static int pwd_data_itit_status = 0;

#ifdef PWD_DATA_ARRAY
static struct passwd *pwd_data = NULL;
#endif
#ifdef PWD_DATA_LIST
struct pwd_data_node {
    struct passwd pwd;
    struct pwd_data_node *next;
};
static struct pwd_data_node *pwd_data = NULL;
#endif
#ifdef PWD_DATA_LIST
static struct pwd_data_node *curpwd = NULL;
#define CURPWD curpwd->pwd
#endif
#ifdef PWD_DATA_ARRAY
static size_t pwd_iter;
#define CURPWD pwd_data[pwd_iter]
static size_t pwd_data_size = 0;
#endif
#ifdef PWD_DATA_LIST
static struct pwd_data_node *pwd_list_add(struct pwd_data_node *where, struct pwd_data_node *what) {
    if(where == NULL)
        return what;
    if(where->next == NULL)
        where->next = what;
    else
        where->next = pwd_list_add(where->next, what);
    return where;
}
#endif
static void struct_passwd_dupfree(struct passwd *pwp) {
    if(pwp->pw_name)
        free(pwp->pw_name);
    if(pwp->pw_passwd)
        free(pwp->pw_passwd);
    if(pwp->pw_gecos)
        free(pwp->pw_gecos);
    if(pwp->pw_dir)
        free(pwp->pw_dir);
    if(pwp->pw_shell)
        free(pwp->pw_shell);
}
static int struct_passwd_dupcopy(struct passwd *where, struct passwd *what) {

    if(what->pw_name) {
        if((where->pw_name = strdup(what->pw_name)) == NULL)
            goto err_exit;
    } else {
        where->pw_name = NULL;
    }
    if(what->pw_passwd) {
        if((where->pw_passwd = strdup(what->pw_passwd)) == NULL)
            goto err_exit;
    } else {
        where->pw_passwd = NULL;
    }

    where->pw_uid = what->pw_uid;
    where->pw_gid = what->pw_gid;

    if(what->pw_gecos) {
        if((where->pw_gecos = strdup(what->pw_gecos)) == NULL)
            goto err_exit;
    } else {
        where->pw_gecos = NULL;
    }
    if(what->pw_dir) {
        if((where->pw_dir = strdup(what->pw_dir)) == NULL)
            goto err_exit;
    } else {
        where->pw_dir = NULL;
    }
    if(what->pw_shell) {
        if((where->pw_shell = strdup(what->pw_shell)) == NULL)
            goto err_exit;
    } else {
        where->pw_shell = NULL;
    }
    return 0;
err_exit:
    struct_passwd_dupfree(where);
    PDBG(LOG_ERR, "struct_passwd_mcp() failed");
    return -1;
}
static int pwd_data_add(struct passwd *pwp) {
#ifdef PWD_DATA_ARRAY
    pwd_data_size++;
    struct passwd *pwd_data_old;
    if((pwd_data = realloc(&pwd_data_old, sizeof(struct pwd_data)*pwd_data_size)) != NULL) {
    }
#endif
#ifdef PWD_DATA_LIST
    struct pwd_data_node *node;
    if((node = malloc(sizeof(struct pwd_data_node))) == NULL) {
        return -1;
    }
    node->next = NULL;
    if(struct_passwd_dupcopy(&node->pwd, pwp) < 0) {
        free(node);
        return -1;
    }
    pwd_data = pwd_list_add(pwd_data, node);
    //PDBG(LOG_DEBUG, "node(%s) added, pwd_data = %p", node->pwd.pw_name, pwd_data);
    //PDBG(LOG_INFO, "%s (%d)\tHOME %s\tSHELL %s\n", pwp->pw_name,
    //                  pwp->pw_uid, pwp->pw_dir, pwp->pw_shell);

#endif

}
static pthread_mutex_t pwd_lock = PTHREAD_MUTEX_INITIALIZER;

static int pwd_file_handler(const char *arg) {
    FILE *pwd_file;
    struct passwd *pwp;

    if((pwd_file = fopen(arg, "r")) == NULL) {
        PDBG(LOG_ERR, "Can't fopen() %s: %s", arg, strerror(errno));
        return -1;
    }

    while((pwp = fgetpwent(pwd_file)) != NULL) {
        pwd_data_add(pwp);
    }
    if(errno != 0)
        PDBG(LOG_ERR, "Can't fgetpwent(): %s", strerror(errno));
    fclose(pwd_file);

    return 0;
}
static int pwd_exec_handler(const char *arg) {
    FILE *pwd_file;
    struct passwd *pwp;
    char *buf;
    char *tmp;
    int ret = 0;
    if((tmp = tmpnam(NULL)) == NULL) {
        PDBG(LOG_ERR, "Can't tmpnam(): %s", strerror(errno));
        return -1;
    }
    if((buf = malloc(strlen(arg) + sizeof(" > ") + strlen(tmp))) == NULL) {
        PDBG(LOG_ERR, "Can't malloc(): %s", strerror(errno));
        return -1;
    }
    sprintf(buf, "%s > %s", arg, tmp);
    if(system(buf) < 0) {
        PDBG(LOG_ERR, "Can't system(%s): %s", buf, strerror(errno));
        ret = -1;
        goto err_buf_file;
    }
/*
    if((pwd_file = fopen(tmp, "r")) == NULL) {
        PDBG(LOG_ERR, "Can't fopen() %s: %s", tmp, strerror(errno));
        goto err_buf;
    }

    while((pwp = fgetpwent(pwd_file)) != NULL) {
        pwd_data_add(pwp);
    }
    if(errno != 0)
        PDBG(LOG_ERR, "Can't fgetpwent(): %s", strerror(errno));

    fclose(pwd_file);

*/
    ret = pwd_file_handler(tmp);
    
err_buf_file:
    if(unlink(tmp) < 0) {
        PDBG(LOG_ERR, "Can't unlink(): %s", strerror(errno));
        ret = -1;
    }
err_buf:
    free(buf);
    return 0;
}
static int pwd_data_init() {
    FILE *conf_file;
    int conf_file_line = 0;
    char buf[CONF_FILE_MAX_LINE_LEN];

    if((conf_file = fopen(NSS_CUSTOM_CONF_FILE, "r")) == NULL) {
        PDBG(LOG_ERR, "Can't open " NSS_CUSTOM_CONF_FILE ": %s", strerror(errno));
        pwd_data_itit_status = errno;
#ifdef PWD_DATA_ARRAY
        pwd_data_size = 0;
#endif
#ifdef PWD_DATA_LIST
        pwd_data = NULL;
#endif
        return -1;
    }

    while(!feof(conf_file)) {
        char *nch;
        if(!fgets(buf, CONF_FILE_MAX_LINE_LEN, conf_file)) {
            break;
        }
        //PDBG(LOG_INFO, NSS_CUSTOM_CONF_FILE ":%d: %s", ++conf_file_line, buf);
        if((nch = strchr(buf, '\n')) != NULL)
            *nch = '\0';
        if((nch = strchr(buf, '#')) != NULL)
            *nch = '\0';
        if(*buf == '\0')
            continue;
        char *op = strtok(buf, " \t");
        char *arg = strtok(NULL, "");
        PDBG(LOG_DEBUG, "op = %s, arg = %s", op, arg);
        if(strcmp(op, "exec") == 0) {
            //PDBG(LOG_INFO, "Exec(%s)", arg);
            pwd_exec_handler(arg);
        } else if(strcmp(op, "file") == 0) {
            //PDBG(LOG_INFO, "File(%s)", arg);
            pwd_file_handler(arg);
        } else {
            PDBG(LOG_WARNING, "Error in " NSS_CUSTOM_CONF_FILE ":%d: unknown operation %s", conf_file_line, op);
        }
    }
    fclose(conf_file);
    pwd_data_inited = 1;
    //PDBG(LOG_INFO, "pwd_data inited.");
    return 0;
}

enum nss_status
PREFIX_DEFINED(setpwent)(int stayopen)
{
    if(!pwd_data_inited)
        pwd_data_init();
#ifdef PWD_DATA_ARRAY
    pwd_iter = 0;
#endif
#ifdef PWD_DATA_LIST
    curpwd = pwd_data;
    //PDBG(LOG_DEBUG, "curpwd == %p", curpwd);
#endif
    return NSS_STATUS_SUCCESS;
}


enum nss_status
PREFIX_DEFINED(endpwent)(void)
{
  return NSS_STATUS_SUCCESS;
}


//enum nss_status PREFIX_DEFINED(getpwent_r)(struct passwd *pwbuf, char *buf,
//                      size_t buflen, int );
enum nss_status
PREFIX_DEFINED(getpwent_r)(struct passwd *result, char *buffer, size_t buflen,
                       int *errnop)
{
  char *cp = buffer;
  int res = NSS_STATUS_SUCCESS;

  pthread_mutex_lock (&pwd_lock);
#ifdef PWD_DATA_ARRAY
  if (pwd_iter >= pwd_data_size)
#endif
#ifdef PWD_DATA_LIST
  //PDBG(LOG_DEBUG, "curpwd == %p, res == %d", curpwd, res);
  if (curpwd == NULL)
#endif
    res = NSS_STATUS_NOTFOUND;
  else
    {
      result->pw_name = COPY_IF_ROOM (CURPWD.pw_name);
      result->pw_passwd = COPY_IF_ROOM (CURPWD.pw_passwd);
      result->pw_uid = CURPWD.pw_uid;
      result->pw_gid = CURPWD.pw_gid;
      result->pw_gecos = COPY_IF_ROOM (CURPWD.pw_gecos);
      result->pw_dir = COPY_IF_ROOM (CURPWD.pw_dir);
      result->pw_shell = COPY_IF_ROOM (CURPWD.pw_shell);

      if (result->pw_name == NULL || result->pw_passwd == NULL
          || result->pw_gecos == NULL || result->pw_dir == NULL
          || result->pw_shell == NULL)
        {
          *errnop = ERANGE;
          res = NSS_STATUS_TRYAGAIN;
        }
#ifdef PWD_DATA_ARRAY
      ++pwd_iter;
#endif
#ifdef PWD_DATA_LIST
      //PDBG(LOG_DEBUG, "curpwd->next == %p", curpwd->next);
      curpwd = curpwd->next;
#endif
    }

  pthread_mutex_unlock (&pwd_lock);

  //PDBG(LOG_DEBUG, "%s() == %d", __FUNCTION__, res);
  return res;
}

enum nss_status
PREFIX_DEFINED(getpwnam_r) (const char *name, struct passwd *result, char *buffer,
                       size_t buflen, int *errnop)
{

    //PDBG(LOG_DEBUG, "%s(%s) pwd data is %s inited", __FUNCTION__, name, pwd_data_inited ? "" : "not");
    if(!pwd_data_inited)
        pwd_data_init();
#ifdef PWD_DATA_ARRAY
    for (size_t idx = 0; idx < npwd_data; ++idx) {
        if (strcmp (pwd_data[idx].pw_name, name) == 0)
#endif

#ifdef PWD_DATA_LIST
    struct pwd_data_node *pwnp;
    for (pwnp = pwd_data; pwnp; pwnp = pwnp->next) {
        //PDBG(LOG_DEBUG, "Test if `%s' is equal to `%s'", name, pwnp->pwd.pw_name);
        if(strcmp(pwnp->pwd.pw_name, name) == 0)
#endif
        {
#ifdef PWD_ATA_ARRAY
            struct passwd *pwp = pwd_data + idx;
#endif
#ifdef PWD_DATA_LIST
            struct passwd *pwp = &pwnp->pwd;
#endif
            char *cp = buffer;
            int res = NSS_STATUS_SUCCESS;

            result->pw_name = COPY_IF_ROOM (pwp->pw_name);
            result->pw_passwd = COPY_IF_ROOM (pwp->pw_passwd);
            result->pw_uid = pwp->pw_uid;
            result->pw_gid = pwp->pw_gid;
            result->pw_gecos = COPY_IF_ROOM (pwp->pw_gecos);
            result->pw_dir = COPY_IF_ROOM (pwp->pw_dir);
            result->pw_shell = COPY_IF_ROOM (pwp->pw_shell);

            if (result->pw_name == NULL || result->pw_passwd == NULL
                || result->pw_gecos == NULL || result->pw_dir == NULL
                || result->pw_shell == NULL)
            {
                *errnop = ERANGE;
                res = NSS_STATUS_TRYAGAIN;
            }

            return res;
        }
    }

    return NSS_STATUS_NOTFOUND;
}

