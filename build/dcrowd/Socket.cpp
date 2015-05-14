/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#define USE_SOCKET_H
#define USE_DBG_H
#define USE_LOGGER_H

#include "dcrowd.h"

#if defined(_UNIX)
#include <sys/ioctl.h>
#define SOCK_LAST_ERROR() errno
#define SOCKET_ERROR -1
#define closesocket close
#define ioctlsocket ioctl
typedef int sioctl_t;
#endif

namespace dcrowd {

/********************* Class TSocket Impl **********/
TSocket::TSocket(bool async, int nWaits)
{
	m_sock			= INVALID_SOCKET;
	m_uSelectFlags  = SF_NONE;
	m_err			= SOCKERR_NONE;

	m_async			= async;
	m_waitsec		= nWaits;

	m_canRead		= false; 
	m_canWrite		= false;

}

TSocket::~TSocket(void)
{
	closeSocket();
}

void TSocket::setAsync(bool val)
{
	int	on = 0;
	m_async = val;
	if (m_async == true) on	= 1;
	ioctl(m_sock, FIONBIO, &on);
}

void TSocket::setWaitSec(int val)
{
	m_waitsec = val;
}

void TSocket::setSockOpt(void)
{
	int tos = IPTOS_THROUGHPUT;
 	setsockopt(m_sock, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos));
}

void TSocket::setReuseAddr(void)
{
    int tmp = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,(const char*)&tmp, sizeof(tmp));  
}

void TSocket::setNonBlock(void)
{
	int flags;
	fcntl(m_sock, F_GETFL, &flags);
	flags |= O_NONBLOCK;
	fcntl(m_sock, F_SETFL, &flags);
}

////////////////////////////////////////////////////
void TSocket::setMaskRead(bool mask)
{
	m_canRead = mask;
}

void TSocket::setMaskWrite(bool mask)
{
	m_canWrite = mask;
}

bool TSocket::getMaskRead(void)
{
	return m_canRead;
}

bool TSocket::getMaskWrite(void)
{
	return m_canWrite;
}
//////////////////////////////////////////////////

void TSocket::attach(int fd)
{
	m_sock = fd;
	setNonBlock();
}

void TSocket::closeSocket(void)
{
	if (m_sock >= 0)
	{
	    Select( SF_NONE );
		close(m_sock);
	    m_sock = INVALID_SOCKET;
	}
}

int TSocket::getHandle(void)
{
	if (m_sock < 0)
		return -ENODATA;

	return m_sock;
}

void TSocket::setTimeout(int sec)
{
	struct timeval to;
	to.tv_sec	= sec;
	to.tv_usec	= 0;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
}

struct sockaddr_in TSocket :: getLocalAddr( void ) 
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if( getsockname( m_sock, (sockaddr*)&addr, &len ) == 0 ){}
	return addr;
}

struct sockaddr_in TSocket :: getPeerAddr( void )
{
   	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if( getpeername( m_sock, (sockaddr*)&addr, &len ) == 0 ){}
   	return addr;
}

void TSocket :: Select(UINT32 nWhich)
{
	if(m_uSelectFlags != nWhich)
		m_uSelectFlags = nWhich;
}

UINT32 TSocket :: getSelectFlags(void)
{
	return m_uSelectFlags;
}

int TSocket::__discard_read(int sock, PCHAR buf, UINT nLen)
{
	int ret = -1;
	int size = 0;
	
	do 
	{
		ret = __once_read(sock, buf, nLen);
		if (ret > 0)
			size += ret;
	} while (ret > 0);
	
	return size;
}

int TSocket::__once_read(int sock, PCHAR buf, UINT nLen)
{
	int	nCur	= 0;
	int	nRead	= 0;
	int	nLeft	= nLen;
	
	while (nLeft > 0)
	{
		nCur = __waits_read(sock, m_waitsec);
		if (nCur <= 0)
			break;

		nRead = recv(sock, buf, nLeft, 0);
		
		if (nRead < 0)
		{
			if (errno == EINTR)
			{
				nRead = 0;
			}
			else if (m_async == true && errno == EWOULDBLOCK)
			{
				if ((nCur = nLen - nLeft) > 0)
				{
					break;
				}
				else
				{
					nRead = 0;
					__waits_read(sock, m_waitsec);
				}
			}
			else
			{
			//	printf("recv() return -1\n");
				LOG(1)("Recv Data failure.\n");
				return -1;
			}
		}
		else if (nRead == 0)
		{
			break;
		}
		
		nLeft	-= nRead;
		buf	+= nRead;
		
		if (nLeft <= 0)
			break;
	}
	
	nCur = (nLen - nLeft);
	if (nCur < (int)nLen)
		*buf = '\0';
	
	return nCur;
}

int TSocket::__exact_read(int sock, PCHAR buf, UINT nLen)
{
	int	nCur	= 0;
	int	nRead	= 0;
	int	nLeft	= nLen;
	
	while (nLeft > 0)
	{
		nRead = recv(sock, buf, nLeft, 0);
		
		if (nRead < 0)
		{
			if (errno == EINTR)
			{
				nRead = 0;
			}
			else if (m_async == true && errno == EWOULDBLOCK)
			{
				if ((nCur = nLen - nLeft) > 0)
				{
					break;
				}
				else
				{
					nRead = 0;
					__waits_read(sock, m_waitsec);
				}
			}
			else
			{
				//printf("recv() return -1\n");
				LOG(1)("Recv Data failure.\n");
				return -1;
			}
		}
		else if (nRead == 0)
		{
			break;
		}
		
		nLeft	-= nRead;
		buf		+= nRead;
	}
	
	nCur = (nLen - nLeft);
	if (nCur < (int)nLen)
		*buf = '\0';
	
	return nCur;
}

int TSocket::__exact_write(int sock, CPCHAR buf, UINT nLen)
{
	int	nWrite	= 0;
	int	nLeft	= nLen;
	
	while (nLeft > 0)
	{
		nWrite = send(sock, buf, nLeft, 0);
		
		if (nWrite <= 0)
		{
			if (errno == EINTR)
			{
				nWrite = 0;
			}
			else if (m_async == true && errno == EWOULDBLOCK)
			{
				nWrite = 0;
				__waits_write(sock, m_waitsec);
			}
			else
			{
				//perror("send() return -1\n");
				LOG(1)("send() Data failure.\n");
				return -1;
			}
		}
		
		nLeft	-= nWrite;
		buf	  	+= nWrite;
	}
	
	return nLen;
}

int	TSocket::__waits_read(int sock, int nWaits)
{
	int ret = -EINVAL;
	
	fd_set fdset;
	struct timeval timeout = { 0 };
	
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	
	if (nWaits > 0)
	{
		timeout.tv_sec = nWaits / 1000;
		timeout.tv_usec = 0;
	}

	ret = select(sock + 1, &fdset, NULL, NULL,
							(nWaits < 0) ? NULL : &timeout);
	if (ret < 0)
	{
		if (errno > 0)
			ret = -errno;
		goto errout;
	}
	
errout:
	return ret;
}

int	TSocket::__waits_write(int sock, int nWaits)
{
	int ret = -EINVAL;
	
	fd_set fdset;
	struct timeval timeout = { 0 };
	
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	
	if (nWaits > 0)
	{
		timeout.tv_sec = nWaits / 1000;
		timeout.tv_usec = (nWaits % 1000) * 1000;
	}
	ret = select(sock + 1, NULL, &fdset, NULL, 
						(nWaits < 0) ? NULL : &timeout);
	if (ret < 0)
	{
		if (errno > 0)
			ret = -errno;
		goto errout;
	}
	
	ret = 0;
	
errout:
	return ret;
}


UINT TSocket::host2ip(CPCHAR hostname)
{
	UINT addr = INADDR_NONE;
	
	struct hostent *he = NULL;
	struct in_addr **ip = NULL;
	
	if ((NULL == hostname) || ('\0' == hostname))
		goto errout;
	
	he = gethostbyname(hostname);
	if (NULL == he)
		goto errout;
	
	ip = (struct in_addr **)(he->h_addr_list);
	
	if (ip && *ip) addr = ((struct in_addr *)*ip)->s_addr;
	
errout:
	return addr;
}

PCHAR TSocket::host2str(CPCHAR hostname)
{
	char * addr = NULL;
	struct hostent *he = NULL;
	struct in_addr **ip = NULL;
	
	if ((NULL == hostname) || ('\0' == hostname))
		return NULL;
	
	he = gethostbyname(hostname);
	if (NULL == he)
		return NULL;
	
	ip = (struct in_addr **)(he->h_addr_list);
	if (ip && *ip)
		addr = inet_ntoa(*(struct in_addr *)*ip);
	
	return addr;
}


void TSocket :: dispAddr(PCHAR Addr)
{
	u_int8_t split[4];
	u_int32_t ip;
	u_int32_t *x = (u_int32_t *) Addr;
	ip = ntohl(*x);
	split[0] = (ip & 0xff000000) >> 24;
	split[1] = (ip & 0x00ff0000) >> 16;
	split[2] = (ip & 0x0000ff00) >> 8;
	split[3] = (ip & 0x000000ff);
	fprintf(stdout, "%d.%d.%d.%d", split[0], split[1], split[2], split[3]);
}
	
void TSocket :: nsLookup(char * * AddrList, PCHAR HostName)
{
	int i;
	for (i = 0; AddrList[i]; i++)
	{
		dispAddr(AddrList[i]);
	}
   	putchar('\n');
}

sockerr_t TSocket::getLastError( void )
{   
    return m_err;
}   

/**************************************
 * ListenSocket
 **************************************/
ListenSocket::ListenSocket( void ) : TSocket()
{
    // Empty
}

ListenSocket::~ListenSocket( void )
{
    // Empty
}

bool ListenSocket::Listen(UINT16 port)
{
    m_err = SOCKERR_NONE;
    m_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
    if( m_sock == INVALID_SOCKET )
    {
        m_err = SOCK_LAST_ERROR();
        return false;
    }

    setNonBlock();
    setReuseAddr();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
 	if(bind( m_sock, (struct sockaddr *)&addr,sizeof(struct sockaddr_in) ) < 0)
    {
		m_err = SOCK_LAST_ERROR();
		close( m_sock );
        m_sock = INVALID_SOCKET;
		return false;
    }

    if( 0 != listen( m_sock, SOMAXCONN ) )
    {
        m_err = SOCK_LAST_ERROR();
		close( m_sock );
        m_sock = INVALID_SOCKET;
        return false;
    }

	Select(SF_ACCEPT);
	
	return true;
}

int ListenSocket::Accept(std::string& ipaddr, UINT16& port)
{
	struct sockaddr_in addr;
	socklen_t len = (socklen_t)(sizeof(addr));
	memset(&addr, 0, sizeof(addr));

	int sockfd = accept(m_sock, (struct sockaddr*)&addr, &len);

	if(sockfd != -1)
	{
		ipaddr = inet_ntoa(addr.sin_addr);
		port   = ntohs(addr.sin_port);
	}
	return sockfd;
}

int ListenSocket::Accept( void )
{
	struct sockaddr_in addr;
	socklen_t len = (socklen_t)(sizeof(addr));
	memset(&addr, 0, sizeof(addr));
	
	int sockfd = accept(m_sock, (struct sockaddr*)&addr, &len);
	return sockfd;
}

/**************************************
 * TcpSocket
 **************************************/
TcpSocket::TcpSocket( void ) : TSocket()
{
    // Empty
}

TcpSocket::~TcpSocket( void )
{
    // Empty
}

bool TcpSocket::Connect(CPCHAR ipaddr, UINT16 port, int timeout)
{	
	m_err = SOCKERR_NONE;
    m_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
    if( m_sock == INVALID_SOCKET )
    {
        m_err = SOCK_LAST_ERROR();
        return false;
    }

    setNonBlock();
    setReuseAddr();

	int status, noblock, opt;  
	struct sockaddr_in addr;   
	
	memset(&addr, 0, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipaddr);
	addr.sin_port = htons(port);
	
	status = connect(m_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	
	if(status == -1 && noblock != -1 && errno == EINPROGRESS)
	{
		fd_set writefd;
		struct timeval tv;
		FD_ZERO(&writefd);
		FD_SET(m_sock, &writefd);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		status = select((m_sock + 1), NULL, &writefd, NULL, &tv);
		if (status > 0)
		{
			socklen_t arglen = sizeof(int);
			if(getsockopt(m_sock,SOL_SOCKET,SO_ERROR,&status, &arglen) < 0)
				status = errno;
			if (status != 0)
				errno = status, status = -1;

			if (errno == EINPROGRESS) errno = ETIMEDOUT;
		}
		else if (status == 0)
		{
			errno = ETIMEDOUT, status = -1;
		}
		m_err = SOCK_LAST_ERROR();
	}
	

	if (status < 0)
	{
		m_err = SOCK_LAST_ERROR();
		close(m_sock);
        m_sock = INVALID_SOCKET;
		if (errno == ECONNREFUSED)
			return false;
		else  
			return false;

		Select(SF_CONNECT);
	} 
	else  
	{
		setNonBlock();
	}
	
    /* Enable KEEPALIVE, so dead connections could be closed
	 * earlier. Useful in conjuction with TCP kernel tuning
	 * in /proc/sys/net/ipv4/tcp_* files. */
	opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, (int) sizeof(opt));  
	
	return true;
}

bool TcpSocket::Connect(CPCHAR ipaddr, UINT16 port)
{
    m_err = SOCKERR_NONE;
    m_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
    if( m_sock == INVALID_SOCKET )
    {
        m_err = SOCK_LAST_ERROR();
        return false;
    }

    setNonBlock();
    setReuseAddr();
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipaddr);
	addr.sin_port = htons(port);
	
	if (!inet_aton(ipaddr, &addr.sin_addr))
		return false;
	
	if (connect(m_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == 0)
	{
		return true;
	}
	
    int cnxerr = SOCK_LAST_ERROR();
    if( cnxerr != SOCKERR_INPROGRESS && cnxerr != SOCKERR_WOULDBLOCK )
    {
        m_err = cnxerr;
        close( m_sock );
        m_sock = INVALID_SOCKET;
        return false;
    }
    Select( SF_CONNECT );
	
	return true;
}

bool TcpSocket::Connect(UINT ipaddr, UINT16 port)
{
    m_err = SOCKERR_NONE;
    m_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
    if( m_sock == INVALID_SOCKET )
    {
        m_err = SOCK_LAST_ERROR();
        return false;
    }

    setNonBlock();
    setReuseAddr();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipaddr;
	addr.sin_port = htons(port);

	if(connect(m_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == 0)
	{
		return true;
	}

    int cnxerr = SOCK_LAST_ERROR();
    if( cnxerr != SOCKERR_INPROGRESS && cnxerr != SOCKERR_WOULDBLOCK )
    {
        m_err = cnxerr;
        close( m_sock );
        m_sock = INVALID_SOCKET;
        return false;
    }
    Select( SF_CONNECT );

	return true;
}

int TcpSocket::Send(CPCHAR buf, UINT nLen)
{
	return __exact_write(m_sock, buf, nLen);
}

int TcpSocket::Recv(PCHAR buf, UINT nLen)
{
	return __exact_read(m_sock, buf, nLen);
}

int TcpSocket::RecvOut(PCHAR buf, UINT nLen)
{
	return __discard_read(m_sock, buf, nLen);
}

int TcpSocket::RecvOnce(PCHAR buf, UINT nLen)
{
	return __once_read(m_sock, buf, nLen);
}


/**************************************
 * UdpSocket
 **************************************/
UdpSocket::UdpSocket( void ) : TSocket()
{
    // Empty
}

UdpSocket::~UdpSocket( void )
{
    // Empty
}

bool UdpSocket::Connect(CPCHAR ipaddr, UINT16 port)
{
    m_err = SOCKERR_NONE;
    m_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if( m_sock == INVALID_SOCKET )
    {
        m_err = SOCK_LAST_ERROR();
        return false;
    }
	setNonBlock();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipaddr);
	addr.sin_port = htons(port);
	
	if (!inet_aton(ipaddr, &addr.sin_addr))
		return false;

	if ( connect(m_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0 )
    {
		m_err = SOCK_LAST_ERROR();
		return false;
    }

	return true;
}

size_t UdpSocket::SendTo(CPCHAR ipaddr, UINT16 port, CPCHAR buf, UINT nLen)
{
	m_err = SOCKERR_NONE;
	struct sockaddr_in addr;
	socklen_t len = (socklen_t)sizeof(addr);
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipaddr);
	addr.sin_port = htons(port);

	ssize_t n =	sendto(m_sock, buf, nLen, 0, (struct sockaddr *)&addr, len);
	if( n == SOCKET_ERROR )
	{   
		n = 0;
		m_err = SOCK_LAST_ERROR();
		if( m_err != SOCKERR_WOULDBLOCK )
		{   
			n = SOCKERR_EOF;
		}   
	}   
	return n;
}

size_t UdpSocket::RecvFrom(struct sockaddr_in* fromAddr, PCHAR buf, UINT nLen)
{
	m_err = SOCKERR_NONE;
	struct sockaddr_in addr;
	socklen_t salen = (socklen_t)sizeof(addr);
		
	ssize_t n = recvfrom(m_sock, buf, nLen, 0, (struct sockaddr *)&addr, &salen);
    if( n > 0 )
    {
        *fromAddr = addr ;
    }
	else if( n == 0 ) 
	{   
		n = SOCKERR_EOF;
	}   
	else if( n == SOCKET_ERROR )
	{   
		n = 0;
		m_err = SOCK_LAST_ERROR();
		if( m_err != SOCKERR_WOULDBLOCK )
		{   
			n = SOCKERR_EOF;
		}   
	}

	return n;
}

bool UdpSocket::Bind( UINT16 port )
{
	m_err = SOCKERR_NONE;
	m_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
	if( m_sock == INVALID_SOCKET )
	{
		m_err = SOCK_LAST_ERROR();
		return false;
	}   
	setNonBlock();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
 	if( bind( m_sock, (struct sockaddr *)&addr,sizeof(struct sockaddr_in) ) != 0 )
    {
		m_err = SOCK_LAST_ERROR();
		close( m_sock );
        m_sock = INVALID_SOCKET;
		return false;
    }

	return true;
}

}
