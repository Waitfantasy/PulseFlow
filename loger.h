#include <time.h>
#include <stdarg.h>

#define LOG_BUF_MAX_SIZE 400
#define LOG_TIME_BUF_MAX_SIZE 40
#define LOG_FILE_URI_MAX_SIZE 255

static const char *level_names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static void saveLog(int level, const char *logDir, const char *file, int line, const char *fmt, ... TSRMLS_DC) {
    if(level > 5)
        return;

    int logDirLen = strlen(logDir), logDirEndFlag = 0;

    if (logDirLen <= 0)
        return;

    if (logDir[logDirLen - 1] == '/')
        logDirEndFlag = 1;

    char logFileUri[LOG_FILE_URI_MAX_SIZE];

    char *logFilePointer = stpncpy(logFileUri, logDir, LOG_FILE_URI_MAX_SIZE);

    if (logFilePointer == NULL)
        return;

    char logBuf[LOG_BUF_MAX_SIZE];

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    if (logDirEndFlag) {
        strftime(logFilePointer, sizeof(logFileUri) - strlen(logDir), "%Y-%m-%d.log", lt);
    } else {
        strftime(logFilePointer, sizeof(logFileUri) - strlen(logDir), "/%Y-%m-%d.log", lt);
    }

    FILE *fp = fopen(logFileUri, "ab");

    if (fp) {

        va_list args;
        char buf[LOG_TIME_BUF_MAX_SIZE];

        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';

        int pointer = snprintf(logBuf, LOG_BUF_MAX_SIZE, "%s %-5s %s:%d: ", buf, level_names[level], file, line);

        if (pointer > 0) {

            va_start(args, fmt);

            int tempPointer = vsnprintf(&logBuf[pointer], LOG_BUF_MAX_SIZE - pointer, fmt, args);

            if (tempPointer > 0) {
                pointer += tempPointer;
            }

            va_end(args);

        }


        if (pointer > 0) {
            fprintf(fp, "%s\n", logBuf);
        }

        fclose(fp);
    }
}