/**
 * @copyright   		Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef __DCROWD_H__
#define __DCROWD_H__

#include <stdexcept>
#include <utility>
#include <exception>
#include <new>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <iomanip>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <string>

using namespace std;

#include <cstdarg>
#include <clocale>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <climits>
#include <cassert>
#include <cmath>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <locale.h>

#include <libintl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include "Types.h"

#ifdef USE_LOGGER_H
#include "Logger.h"
#endif

#ifdef USE_HTTPMSG_H
#include "HttpMsg.h"
#endif

#ifdef USE_SOCKET_H
#include "Socket.h"
#endif 

#ifdef USE_POLLER_H
#include "Poller.h"
#endif

#ifdef USE_UTILS_H
#include "Utils.h"
#endif

#ifdef USE_CROWD_SESSION_H
#include "CrowdSession.h"
#endif

#ifdef USE_CROWD_CLIENT_H
#include "CrowdClient.h"
#endif

#endif
