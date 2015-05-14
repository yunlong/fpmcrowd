/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef __POLLER_H__
#define __POLLER_H__

namespace dcrowd {

typedef struct epoll_event TPollEvent;

class TPoller
{
private:
	TPollEvent*		m_pevents;
	int 			m_nevents;
	int  		 	m_epfd;
public:
	TPoller(void): m_pevents(NULL), m_epfd(-1){}
	~TPoller(void){}

	TPollEvent* getPollEvents(void);

	void addSocket(TSocket* skp);
	void delSocket(TSocket* skp);
	void modSocket(TSocket* skp);
	
	bool start(void);
	void stop(void);
	int poll( int timeout = -1);

};

}

#endif
