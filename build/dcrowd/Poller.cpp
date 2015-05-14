/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#define USE_SOCKET_H
#define USE_POLLER_H

#include "dcrowd.h"

#define NEVENT 32000
#define MAX_EPOLL_TIMEOUT (35*60*1000)

namespace dcrowd {

bool TPoller :: start(void)
{
    int m_nevents = NEVENT;
	
	struct rlimit r1;
    if (getrlimit(RLIMIT_NOFILE, &r1) == 0 && r1.rlim_cur != RLIM_INFINITY)
	{
		m_nevents = r1.rlim_cur - 1;
    }

//	cout << m_nevents << endl;

    m_epfd = epoll_create(m_nevents);

	if(m_epfd == -1)
		return false;

    m_pevents = new TPollEvent[m_nevents];
	return true;
}

void TPoller :: stop(void)
{
	close(m_epfd);
	m_epfd = -1;

	if(m_pevents != NULL) 
		delete[] m_pevents;
}

void TPoller::addSocket( TSocket * skp )
{
	struct epoll_event ee;
	ee.events = 0;
	
	if(skp->getMaskRead())
	{
		ee.events = ee.events | EPOLLIN;
	}
	
	if(skp->getMaskWrite())
	{
		ee.events = ee.events | EPOLLOUT;
	}

	ee.data.ptr = skp;
	epoll_ctl(m_epfd, EPOLL_CTL_ADD, skp->getHandle(), &ee);
	
}

void TPoller :: delSocket( TSocket * skp )
{
	epoll_ctl(m_epfd, EPOLL_CTL_DEL, skp->getHandle(), NULL);
}

void TPoller :: modSocket( TSocket * skp )
{
	struct epoll_event ee;
	ee.events = 0;
	
	if(skp->getMaskRead())
	{
		ee.events = ee.events | EPOLLIN;
	}

	if(skp->getMaskWrite())
	{
		ee.events = ee.events | EPOLLOUT;
	}

	ee.data.ptr = skp;

	epoll_ctl(m_epfd, EPOLL_CTL_MOD, skp->getHandle(), &ee);	
}

TPollEvent * TPoller :: getPollEvents(void)
{
	return m_pevents;
}

int TPoller :: poll( int timeout )
{
    if (timeout > MAX_EPOLL_TIMEOUT) 
	{
        /* Linux kernels can wait forever if the timeout is too big;*/
        timeout = MAX_EPOLL_TIMEOUT;
    }
	int	rc = epoll_wait(m_epfd, m_pevents, m_nevents, timeout);
	return rc;
}

}

