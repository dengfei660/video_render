#ifndef __TOOLS_LOGGER_H__
#define __TOOLS_LOGGER_H__


#define LOG_LEVEL_FATAL   0
#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_TRACE1  4
#define LOG_LEVEL_TRACE2  5
#define LOG_LEVEL_TRACE3  6

void logPrint( int level, const char *fmt, ... );

/**
 * @brief set log level
 * the log level from 0 ~ 6
 * @param setLevel the log level
 * 0 - FATL / ERROR
 * 1.- WARN
 * 2.- INFO
 * 3.- DEBUG
 * 4.- TRACE1
 * 5.- TRACE2
 * 6.- TRACE3
 */
void Logger_set_level(int setLevel);

/**
 * @brief get loger level
 *
 * @return int the log level from 0~6
 */
int Logger_get_level();

/**
 * @brief set log file,the filepath must is a absolute path,
 * if set filepath null, will close file and print log to stderr
 * the stderr is a default print out file
 * @param filepath the file absolute path
 */
void Logger_set_file(char *filepath);

#define INT_FATAL(FORMAT, ...)      logPrint(0, "FATAL: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_ERROR(FORMAT, ...)      logPrint(0, "ERROR: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_WARNING(FORMAT, ...)    logPrint(1, " WARN: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_INFO(FORMAT, ...)       logPrint(2, " INFO: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)      logPrint(3, "DEBUG: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_TRACE1(FORMAT, ...)     logPrint(4, "TRACE: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_TRACE2(FORMAT, ...)     logPrint(5, "TRACE: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_TRACE3(FORMAT, ...)     logPrint(6, "TRACE: %s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_TRACE4(FORMAT, ...)     logPrint(6, "TRACE: %s,%s:%d " FORMAT "\n",TAG,__FILE__, __LINE__, __VA_ARGS__)

#define FATAL(...)                  INT_FATAL(__VA_ARGS__, "")
#define ERROR(...)                  INT_ERROR(__VA_ARGS__, "")
#define WARNING(...)                INT_WARNING(__VA_ARGS__, "")
#define INFO(...)                   INT_INFO(__VA_ARGS__, "")
#define DEBUG(...)                  INT_DEBUG(__VA_ARGS__, "")
#define TRACE1(...)                 INT_TRACE1(__VA_ARGS__, "")
#define TRACE2(...)                 INT_TRACE2(__VA_ARGS__, "")
#define TRACE3(...)                 INT_TRACE3(__VA_ARGS__, "")

#endif /*__TOOLS_LOGGER_H__*/