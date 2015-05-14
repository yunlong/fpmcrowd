/**
 * @copyright   		Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@hotmail.com
 * @version          1.0beta
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

namespace dcrowd {

extern time_t curTime;

#define LOG(level) if (level <= Logger::logLevel) Logger::print

class Logger {
private:
	static FILE *logFile;
public:
	static int logLevel;
public:
	static void open(const char *file, int level);
	static void closeLog();
	static void print(const char *fmt, ...);

};

}


#endif
