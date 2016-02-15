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
#include <shadow.h>
#include <grp.h>
#include <nss.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "config.h"



typedef enum {
    NSS_CUSTOM_FILE,
    NSS_CUSTOM_PIPE
} FileType;

typedef struct {
    FileType type;
    char *arg;
    FILE *file;
} ConfigElem;

#define CONFIG_INC 16
typedef struct {
    pthread_mutex_t lock;
    const char *prefix;
    ConfigElem *elems;
    size_t  allocated;
    size_t  last;
    bool    inited;
    size_t  cur;
} Config;

typedef enum {
    NSS_CUSTOM_PW,
    NSS_CUSTOM_SP,
    NSS_CUSTOM_GR
} EntType;
#define CONFIGS 3
    
static Config configs[CONFIGS] = {
    [NSS_CUSTOM_PW] = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .prefix = "pw"
    },
    [NSS_CUSTOM_SP] = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .prefix = "sp"
    },
    [NSS_CUSTOM_GR] = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .prefix = "gr"
    },
};
#define LOCK(T) \
    pthread_mutex_lock(&configs[NSS_CUSTOM_ ## T].lock);

#define UNLOCK(T) \
     pthread_mutex_unlock(&configs[NSS_CUSTOM_ ## T].lock);

#define CFG_INIT(T) do { \
    if (!configs[NSS_CUSTOM_ ## T].inited) \
        config_init(configs + NSS_CUSTOM_ ## T); \
    if (!configs[NSS_CUSTOM_ ## T].inited) \
        return NSS_STATUS_UNAVAIL; \
}while(0)

#define CFG_FREE(T) \
    config_free(configs + NSS_CUSTOM_ ## T)

#define CFG_EL_INIT(ELP) \
    ({ \
        if (!(ELP)->file) { \
            switch((ELP)->type) { \
                case NSS_CUSTOM_FILE: \
                    (ELP)->file = fopen((ELP)->arg, "r"); \
                    break; \
                case NSS_CUSTOM_PIPE: \
                    (ELP)->file = popen((ELP)->arg, "r"); \
                    break; \
            } \
        } \
        (ELP)->file; \
    })
#define CFG_EL_FREE(ELP) \
    do { \
        switch ((ELP)->type) { \
            case NSS_CUSTOM_FILE: \
                fclose((ELP)->file); \
                break; \
            case NSS_CUSTOM_PIPE: \
                pclose((ELP)->file); \
                break; \
        } \
    } while(0)
static int config_append(Config *cfg, ConfigElem el) {
    if (cfg->last == cfg->allocated) {
        cfg->allocated += CONFIG_INC;
        ConfigElem *n = realloc(cfg->elems, sizeof(ConfigElem) * cfg->allocated);
        if (n == NULL)
            return -1;
        cfg->elems == n;
    }
    ConfigElem *cel = cfg->elems + cfg->last;
    cel->type = el.type;
    cel->arg = strdup(el.arg);
    cel->file = NULL;
    if (cel->arg == NULL) {
        return -1;
    }
    cfg->last++;
    return 0;
}
static int config_free(Config *cfg) {
    size_t i;
    for (i = 0; i < cfg->last; i++) {
        if (cfg->elems[i].arg) {
            free(cfg->elems[i].arg);
        }
    }
    if (cfg->elems)
        free(cfg->elems);
    cfg->elems = NULL;
    cfg->allocated = 0;
    cfg->last = 0;
}

static int config_init(Config *cfg) {
    FILE *conf_file;
    int conf_file_line = 0;
    char buf[CONF_FILE_MAX_LINE_LEN];

    if((conf_file = fopen(NSS_CUSTOM_CONF_FILE, "r")) == NULL) {
        PDBG(LOG_ERR, "Can't open " NSS_CUSTOM_CONF_FILE ": %s", strerror(errno));
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
        char *saveptr = NULL;
        char *type = strtok_r(buf, " \t", &saveptr);
        char *op = strtok_r(NULL, " \t", &saveptr);
        char *arg = strtok_r(NULL, "", &saveptr);
        PDBG(LOG_DEBUG, "type = %s, op = %s, arg = %s", type, op, arg);
        if (type == cfg->prefix) {
            if(strcmp(op, "exec") == 0) {
                config_append(cfg, (ConfigElem){ NSS_CUSTOM_PIPE, arg});
            } else if(strcmp(op, "file") == 0) {
                config_append(cfg, (ConfigElem){ NSS_CUSTOM_FILE, arg});
            } else {
                PDBG(LOG_WARNING, "Error in " NSS_CUSTOM_CONF_FILE ":%d: unknown operation %s", conf_file_line, op);
            }
        }
    }
    fclose(conf_file);
    cfg->inited = true;
    cfg->cur = true;
    //PDBG(LOG_INFO, "pwd_data inited.");
    return 0;
}


#if 0

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
#endif


#define DEFINE_ENT_FUNCTIONS(T, TS, TYPE) \
    enum nss_status \
    PREFIX_DEFINED(set ## TS ## ent)(int stayopen) \
    { \
        LOCK(T); \
        CFG_INIT(T); \
        UNLOCK(T); \
        return NSS_STATUS_SUCCESS; \
    } \
\
    enum nss_status \
    PREFIX_DEFINED(end ## TS ## ent)(void) \
    { \
        LOCK(T); \
        CFG_FREE(T); \
        UNLOCK(T); \
        return NSS_STATUS_SUCCESS; \
    } \
\
    enum nss_status \
    PREFIX_DEFINED(get ## TS ## ent_r)(TYPE *result, char *buffer, size_t buflen, \
                           int *errnop) \
    { \
        char *cp = buffer; \
        enum nss_status ret = NSS_STATUS_SUCCESS; \
        int err; \
\
        LOCK(T); \
        CFG_INIT(T); \
\
        Config *cfg = configs + NSS_CUSTOM_ ## T; \
        bool found = false; \
        do { \
            PDBG(LOG_INFO, "%s() %zu/%zu", __FUNCTION__, cfg->cur, cfg->last); \
            if (cfg->cur == cfg->last) { \
                ret = NSS_STATUS_NOTFOUND; \
                *errnop = ENOENT; \
                found = true; \
            } else { \
                TYPE *pwp; \
                ConfigElem *el = cfg->elems + cfg->cur; \
                PDBG(LOG_INFO, "%s() %s", __FUNCTION__, el->arg); \
                if (!CFG_EL_INIT(el)) { \
                    PDBG(LOG_ERR, "Can't use %s: %s", el->arg, strerror(errno)); \
                    cfg->cur++; \
                    continue; \
                } \
                err = fget ## TS ## ent_r(el->file, result, buffer, buflen, &pwp); \
                PDBG(LOG_INFO, "%s() fget*ent_t", __FUNCTION__, err); \
                switch (err) { \
                    case 0: /* OK */ \
                        found = true; \
                        break; \
                    case ERANGE: /* Buffer too small */ \
                        ret = NSS_STATUS_TRYAGAIN; \
                        *errnop = err; \
                    case ENOENT: \
                    default: \
                        /* Let's read another */ \
                        CFG_EL_FREE(el); \
                        cfg->cur++; \
                        break; \
                }; \
            } \
        } while(!found); \
\
        UNLOCK(T); \
        return ret; \
    }

DEFINE_ENT_FUNCTIONS(PW, pw, struct passwd);
DEFINE_ENT_FUNCTIONS(SP, sp, struct spwd);
DEFINE_ENT_FUNCTIONS(GR, gr, struct group);

#define DEFINE_NAM_FUNCTIONS(T,TS,TYPE,N) \
    enum nss_status \
    PREFIX_DEFINED(get ## TS ## nam_r) (const char *name, TYPE *result, char *buffer, \
                           size_t buflen, int *errnop) \
    { \
        PREFIX_DEFINED(set ## TS ## ent)(0); \
        TYPE *pwp; \
        for(;;) { \
            int err; \
            PDBG(LOG_INFO, "%s(%s)", __FUNCTION__, name); \
            enum nss_status ret = PREFIX_DEFINED(get ## TS ## ent_r)(result, buffer, buflen, &err); \
            switch (ret) { \
                case NSS_STATUS_SUCCESS: \
                    break; \
                case NSS_STATUS_TRYAGAIN: \
                case NSS_STATUS_NOTFOUND: \
                default: \
                    *errnop = err; \
                    memset(result, 0, sizeof(TYPE)); \
                    memset(buffer, 0, buflen); \
                    PREFIX_DEFINED(end ## TS ## ent)(); \
                    return ret; \
                    break; \
            } \
            if (strcmp(result->N, name) == 0) { \
                PREFIX_DEFINED(end ## TS ## ent)(); \
                return NSS_STATUS_SUCCESS; \
            } \
        } \
        PREFIX_DEFINED(end ## TS ## ent)(); \
        return NSS_STATUS_NOTFOUND; \
    }
DEFINE_NAM_FUNCTIONS(PW, pw, struct passwd, pw_name);
DEFINE_NAM_FUNCTIONS(SP, sp, struct spwd, sp_namp);
DEFINE_NAM_FUNCTIONS(GR, gr, struct group, gr_name);
