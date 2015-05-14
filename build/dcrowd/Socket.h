/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef	__SOCKET_H__
#define	__SOCKET_H__

#define _UNIX

/////////////////////////////////////////////////////////////////////
// some custom bits (we rely on pollfd events being a 16-bit type)
// note that these are combined with POLLIN/POLLOUT
#ifdef _UNIX
#define XPOLLACC    0x00010000  /* accept */
#define XPOLLCNX    0x00020000  /* connect */
#define INFTIM (-1)

#include <sys/poll.h>
typedef pollfd	waitobj_t;
typedef unsigned int  waitevents_t;

#define WAITOBJ_MAX          		0xffff

#define WAITOBJ_IS_VALID(obj)       (obj.fd != -1)
#define WAIT_EVENT_ACCEPT(wevt)     (wevt & XPOLLACC)
#define WAIT_EVENT_CONNECT(wevt)    (wevt & XPOLLCNX)
#define WAIT_EVENT_READ(wevt)       (wevt & EPOLLIN)
#define WAIT_EVENT_WRITE(wevt)      (wevt & EPOLLOUT)
#define WAIT_EVENT_EXCEPT(wevt)     (wevt & EPOLLPRI)

#endif

/////////////////////////////////////////////////////////////////////
#ifdef _UNIX
typedef int sockobj_t;
typedef int sockerr_t;

#define INVALID_SOCKET -1
#define SF_NONE     0
#define SF_ACCEPT   (XPOLLACC|EPOLLIN)
#define SF_CONNECT  (XPOLLCNX|EPOLLOUT)
#define SF_READ     EPOLLIN
#define SF_WRITE    EPOLLOUT
#define SF_EXCEPT   EPOLLPRI
#define SF_ERR		EPOLLERR
#define SF_HUP		EPOLLHUP
#define SF_ALL      (XPOLLACC|XPOLLCNX|EPOLLIN|EPOLLOUT|EPOLLPRI)

#define SOCKERR_NONE        0
#define SOCKERR_WOULDBLOCK  EAGAIN
#define SOCKERR_INPROGRESS  EINPROGRESS
#define SOCKERR_CONNRESET   EPIPE
#define SOCKERR_EOF         0x7FFFFFFF
#endif
/////////////////////////////////////////////////////////////////////
#define INVALID_PORT 		0xffff
#define MAX_UDP_LEN 		8192 

namespace dcrowd {

/********************* Class TSocket Definition **********/
typedef enum __SockType {
	SKT_TCP			= 0,
	SKT_UDP			= 1,
	SKT_RAW			= 2
} SockType;

class TSocket 
{
private:
//	int			m_sock;
//	UINT32      m_uSelectFlags;
//	sockerr_t   m_err; 

	int 	m_waitsec;
	bool	m_async;
	
	bool 	m_canRead;
	bool 	m_canWrite;
	
	
protected:
    sockobj_t   m_sock;
    UINT32      m_uSelectFlags;
    sockerr_t   m_err;

public:
	TSocket(bool async = false, int nWaits = 3000);
	virtual ~TSocket(void);
	
	void setNonBlock(void);	
	void setReuseAddr(void);
	void setSockOpt(void);
	void setIPTos(void);
	
	void attach(int fd);
	int getHandle(void);
	sockerr_t getLastError( void );

	/*****************************************/
	struct sockaddr_in getLocalAddr( void ); 
	struct sockaddr_in getPeerAddr( void );
	void Select(UINT32 nWhich);
	UINT32	getSelectFlags(void);
	void closeSocket(void);

public :

	void dispAddr(PCHAR Addr);
	void nsLookup(char * * AddrList, PCHAR HostName);

	UINT	host2ip(CPCHAR hostname);
	PCHAR	host2str(CPCHAR hostname);
	
	void	setAsync(bool async);
	void 	setWaitSec(int val);
	void	setTimeout(int sec);

	////////////////////////////////////////////////////
	int __once_read(int sock, PCHAR buf, UINT nLen);	
	int __discard_read(int sock, PCHAR buf, UINT nLen);
	int	__exact_read(int sock, PCHAR buf, UINT nLen);
	int	__exact_write(int sock, CPCHAR buf, UINT nLen);
	int	__waits_read(int sock, int nWaits = 3000);
	int	__waits_write(int sock, int nWaits = 3000);

public:
	/******** Reserved for PeerConnection ******/
	void setMaskRead(bool mask);
	bool getMaskRead(void);
	void setMaskWrite(bool mask);
	bool getMaskWrite(void);
	virtual void handleClose(void) {}
	virtual void handleRead(void) {}
	virtual void handleWrite(void) {}
};

class ListenSocket : public TSocket
{
public:
    ListenSocket( void );
    virtual ~ListenSocket( void );
public:
	int Accept(std::string& ipaddr, UINT16& port); 
	int Accept(void); 
    bool Listen( UINT16 port );
};

class TcpSocket : public TSocket
{
public:
    TcpSocket( void );
    virtual ~TcpSocket( void );
public:
	bool Connect(UINT ipaddr, UINT16 port);
	bool Connect(CPCHAR ipaddr, UINT16 port);
	bool Connect(CPCHAR ipaddr, UINT16 port, int timeout = 100);
public:
	int 	Send(CPCHAR buf, UINT nLen);
	int 	Recv(PCHAR buf, UINT nLen);
	int		RecvOut(PCHAR buf, UINT nLen);
	int 	RecvOnce(PCHAR buf, UINT nLen);
};

class UdpSocket : public TSocket
{
public:
    UdpSocket( void );
    virtual ~UdpSocket( void );
public:

    bool   Bind( UINT16 port );
	bool Connect(CPCHAR ipaddr, UINT16 port);
	size_t 	SendTo(CPCHAR ipaddr, UINT16 port, CPCHAR buf, UINT nLen);
	size_t 	RecvFrom(struct sockaddr_in * addr,PCHAR buf, UINT nLen);
	
};

}

#endif


	
