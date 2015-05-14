/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@hotmail.com
 * @version          1.0beta
 */

#define USE_LOGGER_H
#include "dcrowd.h"

#define BUFSIZE		4096

namespace dcrowd {

pthread_mutex_t LogLock = PTHREAD_MUTEX_INITIALIZER;
FILE *Logger::logFile = NULL;
int Logger::logLevel = 0;
time_t curTime;

void Logger::open(const char *file, int level)
{
	logLevel = level;

	if (logFile)
		closeLog();

	bool b = (file && *file);

	if(b) logFile = fopen(file, "a+");
	if(!logFile) 
	{
		logFile = stderr;
		if (b) LOG(1)("Error open log file: %s\n", file);
	}
}

void Logger::closeLog()
{
	if (logFile && logFile != stderr) 
	{
		fclose(logFile);
		logFile = NULL;
	}
}

void Logger::print(const char *fmt, ...)
{
	pthread_mutex_lock(&LogLock);

   	char ct[128];
	struct tm *tm = localtime(&curTime);
	strftime(ct, 127, "%Y/%m/%d %H:%M:%S", tm);
	
	char logbuf[BUFSIZE];
	va_list args;
	va_start(args, fmt);
	snprintf(logbuf, sizeof(logbuf), "%s| %s", ct, fmt);
	vfprintf(logFile, logbuf, args);
	fflush(logFile);
	va_end(args);

	pthread_mutex_unlock(&LogLock);

}

}
