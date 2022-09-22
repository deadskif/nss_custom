#include <stdio.h>
#include <pwd.h>

int main(int argc, const char *argv[])
{
    struct passwd *p = NULL;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <name>\n", argv[0]);
        return 1;
    }

    if ((p = getpwnam(argv[1])) != NULL) {
        printf("%s (%d)\tHOME %s\tSHELL %s\n", p->pw_name,
            p->pw_uid, p->pw_dir, p->pw_shell);
    }
    return 0;
}
