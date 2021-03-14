#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/types.h>

#include "chipid.h"
#include "firmware.h"
#include "uboot.h"

static unsigned long time_by_proc(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return 0;

    char line[1024];
    if (!fgets(line, sizeof(line), fp))
        return 0;
    const char *rpar = strchr(line, ')');
    if (!rpar || *(rpar + 1) != ' ')
        return 0;

    char state;
    long long ppid, pgid, sid, tty_nr, tty_pgrp;
    unsigned long flags, min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stime;
    long long cutime, cstime;
    sscanf(rpar + 2,
           "%c %lld %lld %lld %lld %lld %ld %ld %ld %ld %ld %ld %ld %lld %lld",
           &state, &ppid, &pgid, &sid, &tty_nr, &tty_pgrp, &flags, &min_flt,
           &cmin_flt, &maj_flt, &cmaj_flt, &utime, &stime, &cutime, &cstime);

    fclose(fp);
    return utime + stime;
}

static void get_god_app() {
    DIR *dir = opendir("/proc");
    if (!dir)
        return;

    unsigned long max = 0;
    char sname[1024];
    pid_t godpid;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (*entry->d_name && isdigit(entry->d_name[0])) {
            snprintf(sname, sizeof(sname), "/proc/%s/stat", entry->d_name);

            unsigned long curres = time_by_proc(sname);
            if (curres > max) {
                max = curres;
                godpid = strtol(entry->d_name, NULL, 10);
            }
        }
    };
    closedir(dir);

    if (godpid) {
        snprintf(sname, sizeof(sname), "/proc/%d/cmdline", godpid);
        FILE *fp = fopen(sname, "r");
        if (!fp)
            return;
        if (!fgets(sname, sizeof(sname), fp))
            return;
        snprintf(firmware + strlen(firmware),
                 sizeof(firmware) - strlen(firmware), "  god-app: %s\n", sname);

        fclose(fp);
    }
}

bool detect_firmare() {
    const char *uver = uboot_getenv("ver");
    if (uver) {
        const char *stver = strchr(uver, ' ');
        if (stver && *(stver + 1)) {
            snprintf(firmware, sizeof(firmware), "  u-boot: %s\n", stver + 1);
        }
    }

    get_god_app();

    return true;
}