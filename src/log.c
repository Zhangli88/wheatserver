// Implementation of log messages handler
//
// Copyright (c) 2013 The Wheatserver Author. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "wheatserver.h"

#define WHEAT_MAX_LOGFILE    (100*1024*1024)

// TODO: When reload config file if specify logfile is stdout,
// now code can't redirect back to stdout.
void logRedirect()
{
    if (Server.logfile && strcasecmp(Server.logfile, "stdout")) {
        FILE *fp;
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
        fp = freopen(Server.logfile, "a", stdout);
        if (!fp) {
            wheatLog(WHEAT_WARNING, "file redirect failed %s",
                    strerror(errno));
            halt(1);
        }
        setbuf(fp, NULL);
        fp = freopen(Server.logfile, "a", stderr);
        if (!fp) {
            wheatLog(WHEAT_WARNING, "file redirect failed %s",
                    strerror(errno));
            halt(1);
        }
        setbuf(fp, NULL);
    }
}

// Now, logging will open file and flush it to disk if logfile is file, close
// it finally. It may slower process jobs and not effective.
// TODO: Open file and keep file description. Export flush policy to users.
void wheatLogRaw(int level, const char *msg)
{
    static char *c = ".-* #";
    FILE *fp;
    char buf[64];
    off_t len;
    int rawmode = (level & WHEAT_LOG_RAW);

    level &= 0xff; /* clear flags */
    if (level < Server.verbose) return;

    if (Server.logfile == NULL || !strcasecmp(Server.logfile, "stdout")) {
        fp = stdout;
    } else {
        fp = fopen(Server.logfile, "a");
        if (getFileSizeAndMtime(fileno(fp), &len, NULL)) {
            if (len > WHEAT_MAX_LOGFILE) {
                char newname[WHEATSERVER_PATH_LEN];
                time_t now;
                fclose(fp);
                now = time(NULL);
                snprintf(newname, sizeof(newname), "%s.%s", Server.logfile, ctime(&now));
                rename(Server.logfile, newname);
                fp = fopen(Server.logfile, "a");
            }
        }
    }
    if (!fp) return;

    if (rawmode) {
        fprintf(fp,"%s",msg);
    } else {
        size_t off;
        struct timeval tv;

        gettimeofday(&tv,NULL);
        off = strftime(buf, sizeof(buf), "%d %b %H:%M:%S.", localtime(&tv.tv_sec));
        snprintf(buf+off, sizeof(buf)-off, "%03d", (int)tv.tv_usec/1000);
        fprintf(fp,"[%d] %s %c %s\n", (int)getpid(), buf, c[level], msg);
    }
    fflush(fp);

    if (fp != stdout) fclose(fp);
}

// `level`: different log level setting will decide whether log message or not.
// * WHEAT_DEBUG
// * WHEAT_VERBOSE
// * WHEAT_NOTICE
// * WHEAT_WARNING
void wheatLog(int level, const char *fmt, ...)
{
    va_list ap;
    char msg[WHEATSERVER_MAX_LOG_LEN];
    int ret;

    if ((level&0xff) < Server.verbose) return;

    va_start(ap, fmt);
    ret = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    wheatLogRaw(level, msg);
}
