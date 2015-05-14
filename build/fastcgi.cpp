#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "fastcgi.h"

#ifdef __cplusplus 
extern "C" {
#endif

#include "fpm.h"
#include "fpm_request.h"

#ifdef __cplusplus 
}
#endif


#define closesocket(s) close(s)

#include <sys/select.h>

#define  PHP_FASTCGI_PM 1

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

typedef unsigned int socklen_t;

# ifdef USE_LOCKING
#  define FCGI_LOCK(fd)								\
	do {											\
		struct flock lock;							\
		lock.l_type = F_WRLCK;						\
		lock.l_start = 0;							\
		lock.l_whence = SEEK_SET;					\
		lock.l_len = 0;								\
		if (fcntl(fd, F_SETLKW, &lock) != -1) {		\
			break;									\
		} else if (errno != EINTR || in_shutdown) {	\
			return -1;								\
		}											\
	} while (1)

#  define FCGI_UNLOCK(fd)							\
	do {											\
		int orig_errno = errno;						\
		while (1) {									\
			struct flock lock;						\
			lock.l_type = F_UNLCK;					\
			lock.l_start = 0;						\
			lock.l_whence = SEEK_SET;				\
			lock.l_len = 0;							\
			if (fcntl(fd, F_SETLK, &lock) != -1) {	\
				break;								\
			} else if (errno != EINTR) {			\
				return -1;							\
			}										\
		}											\
		errno = orig_errno;							\
	} while (0)
# else
#  define FCGI_LOCK(fd)
#  define FCGI_UNLOCK(fd)
# endif


typedef union _sa_t {
	struct sockaddr     sa;
	struct sockaddr_un  sa_unix;
	struct sockaddr_in  sa_inet;
} sa_t;

typedef struct _fcgi_mgmt_rec {
	char*  name;
	char   name_len;
	char   val;
} fcgi_mgmt_rec;

static const fcgi_mgmt_rec fcgi_mgmt_vars[] = {
	{"FCGI_MAX_CONNS",  sizeof("FCGI_MAX_CONNS")-1,  1},
	{"FCGI_MAX_REQS",   sizeof("FCGI_MAX_REQS")-1,   1},
	{"FCGI_MPXS_CONNS", sizeof("FCGI_MPXS_CONNS")-1, 0}
};

//static int env_cnt = 0;

static int is_initialized = 0;
static int is_fastcgi = 0;
static int in_shutdown = 0;
static in_addr_t *allowed_clients = NULL;

static void fcgi_signal_handler(int signo)
{
	if (signo == SIGUSR1 || signo == SIGTERM) {
		in_shutdown = 1;
	}
}

int fcgi_in_shutdown(void)
{
	return in_shutdown;
}

int fcgi_init(void)
{
	if (!is_initialized) 
	{
		sa_t sa;
		socklen_t len = sizeof(sa);

		is_initialized = 1;
		errno = 0;
		if (getpeername(0, (struct sockaddr *)&sa, &len) != 0 && errno == ENOTCONN) 
		{
			struct sigaction new_sa, old_sa;

			sigemptyset(&new_sa.sa_mask);
			new_sa.sa_flags = 0;
			new_sa.sa_handler = fcgi_signal_handler;
			sigaction(SIGUSR1, &new_sa, NULL);
			sigaction(SIGTERM, &new_sa, NULL);
			sigaction(SIGPIPE, NULL, &old_sa);
			if (old_sa.sa_handler == SIG_DFL) 
			{
				sigaction(SIGPIPE, &new_sa, NULL);
			}

			return is_fastcgi = 1;
		} 
		else 
		{
			return is_fastcgi = 0;
		}

		fcgi_set_allowed_clients(getenv("FCGI_WEB_SERVER_ADDRS"));

	}
	return is_fastcgi;
}


int fcgi_is_fastcgi(void)
{
	if (!is_initialized) {
		return fcgi_init();
	} else {
		return is_fastcgi;
	}
}

void fcgi_set_is_fastcgi(int new_value)
{
	is_fastcgi = new_value;
}

void fcgi_set_in_shutdown(int new_value)
{
	in_shutdown = new_value;
}

void fcgi_shutdown(void)
{
	is_fastcgi = 0;

	if (allowed_clients) {
		free(allowed_clients);
		allowed_clients = 0;
	}
}


void fcgi_set_allowed_clients(char *ip)
{
    char *cur, *end;
    int n;
	    
    if (ip) 
	{
    	ip = strdup(ip);
    	cur = ip;
    	n = 0;
    	while (*cur) 
		{
    		if (*cur == ',') n++;
    		cur++;
    	}
		if (allowed_clients) free(allowed_clients);
    	allowed_clients =(in_addr_t*)malloc(sizeof(in_addr_t) * (n+2));
    	n = 0;
    	cur = ip;

    	while (cur) 
		{
	    	end = strchr(cur, ',');
	    	if (end) 
			{
    			*end = 0;
    			end++;
    		}
    		allowed_clients[n] = inet_addr(cur);
    		if (allowed_clients[n] == INADDR_NONE) 
			{
				fprintf(stderr, "Wrong IP address '%s' in FCGI_WEB_SERVER_ADDRS\n", cur);
    		}
    		n++;
    		cur = end;
    	}
    	allowed_clients[n] = INADDR_NONE;
		free(ip);
	}
}

static int is_port_number(const char *bindpath)
{
	while (*bindpath) 
	{
		if (*bindpath < '0' || *bindpath > '9') 
		{
			return 0;
		}
		bindpath++;
	}
	return 1;
}

#define MAX_PATH_LEN 1024

int fcgi_listen(const char *path, int backlog)
{
	const char     *s;
	int       tcp = 0;
	char      host[MAX_PATH_LEN];
	short     port = 0;
	int       listen_socket;
	sa_t      sa;
	socklen_t sock_len;
#ifdef SO_REUSEADDR
	int reuse = 1;
#endif

	if ((s = strchr(path, ':'))) 
	{
		port = atoi(s+1);
		if (port != 0 && (s-path) < MAX_PATH_LEN) 
		{
			strncpy(host, path, s-path);
			host[s-path] = '\0';
			tcp = 1;
		}
	} 
	else if (is_port_number(path)) 
	{
		port = atoi(path);
		if (port != 0) 
		{
			host[0] = '\0';
			tcp = 1;
		}
	}

	/* Prepare socket address */
	if (tcp) 
	{
		memset(&sa.sa_inet, 0, sizeof(sa.sa_inet));
		sa.sa_inet.sin_family = AF_INET;
		sa.sa_inet.sin_port = htons(port);
		sock_len = sizeof(sa.sa_inet);

		if (!*host || !strncmp(host, "*", sizeof("*")-1)) 
		{
			sa.sa_inet.sin_addr.s_addr = htonl(INADDR_ANY);
		} 
		else 
		{
			sa.sa_inet.sin_addr.s_addr = inet_addr(host);
			if (sa.sa_inet.sin_addr.s_addr == INADDR_NONE) 
			{
				struct hostent *hep;

				hep = gethostbyname(host);
				if (!hep || hep->h_addrtype != AF_INET || !hep->h_addr_list[0]) 
				{
					fprintf(stderr, "Cannot resolve host name '%s'!\n", host);
					return -1;
				}
				else if (hep->h_addr_list[1]) 
				{
					fprintf(stderr, "Host '%s' has multiple addresses. You must choose one explicitly!\n", host);
					return -1;
				}
				sa.sa_inet.sin_addr.s_addr = ((struct in_addr*)hep->h_addr_list[0])->s_addr;
			}
		}
	} 
	else 
	{
		int path_len = strlen(path);

		if (path_len >= sizeof(sa.sa_unix.sun_path)) {
			fprintf(stderr, "Listening socket's path name is too long.\n");
			return -1;
		}

		memset(&sa.sa_unix, 0, sizeof(sa.sa_unix));
		sa.sa_unix.sun_family = AF_UNIX;
		memcpy(sa.sa_unix.sun_path, path, path_len + 1);
		sock_len = (size_t)(((struct sockaddr_un *)0)->sun_path)	+ path_len;
#ifdef HAVE_SOCKADDR_UN_SUN_LEN
		sa.sa_unix.sun_len = sock_len;
#endif
		unlink(path);
	}

	/* Create, bind socket and start listen on it */
	if ((listen_socket = socket(sa.sa.sa_family, SOCK_STREAM, 0)) < 0 ||
#ifdef SO_REUSEADDR
	    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0 ||
#endif
	    bind(listen_socket, (struct sockaddr *) &sa, sock_len) < 0 ||
	    listen(listen_socket, backlog) < 0) 
	{

		fprintf(stderr, "Cannot bind/listen socket - [%d] %s.\n",errno, strerror(errno));
		return -1;
	}

	if (!tcp) 
	{
		chmod(path, 0777);
	}

	if (!is_initialized) 
	{
		fcgi_init();
	}
	is_fastcgi = 1;

	return listen_socket;
}

void fcgi_init_request(fcgi_request *req, int listen_socket)
{
	memset(req, 0, sizeof(fcgi_request));
	req->listen_socket = listen_socket;
	req->fd = -1;
	req->id = -1;

	req->in_len = 0;
	req->in_pad = 0;

	req->out_hdr  = NULL;
	req->out_pos  = req->out_buf;

}

static inline ssize_t safe_write(fcgi_request *req, const void *buf, size_t count)
{
	int    ret;
	size_t n = 0;

	do {
		errno = 0;
		ret = write(req->fd, ((char*)buf)+n, count-n);
		if (ret > 0) 
		{
			n += ret;
		} 
		else if (ret <= 0 && errno != 0 && errno != EINTR) 
		{
			return ret;
		}
	} while (n != count);
	return n;
}

static inline ssize_t safe_read(fcgi_request *req, const void *buf, size_t count)
{
	int    ret;
	size_t n = 0;

	do {
		errno = 0;
		ret = read(req->fd, ((char*)buf)+n, count-n);
		if (ret > 0) 
		{
			n += ret;
		} 
		else if (ret == 0 && errno == 0) 
		{
			return n;
		} 
		else if (ret <= 0 && errno != 0 && errno != EINTR) 
		{
			return ret;
		}
	} while (n != count);
	return n;
}

static inline int fcgi_make_header(fcgi_header *hdr, fcgi_request_type type, int req_id, int len)
{
	int pad = ((len + 7) & ~7) - len;

	hdr->contentLengthB0 = (unsigned char)(len & 0xff);
	hdr->contentLengthB1 = (unsigned char)((len >> 8) & 0xff);
	hdr->paddingLength = (unsigned char)pad;
	hdr->requestIdB0 = (unsigned char)(req_id & 0xff);
	hdr->requestIdB1 = (unsigned char)((req_id >> 8) & 0xff);
	hdr->reserved = 0;
	hdr->type = type;
	hdr->version = FCGI_VERSION_1;
	if (pad) 
	{
		memset(((unsigned char*)hdr) + sizeof(fcgi_header) + len, 0, pad);
	}
	return pad;
}

static int fcgi_get_params(fcgi_request *req, unsigned char *p, unsigned char *end)
{
	char buf[128];
	char *tmp = buf;
	int buf_size = sizeof(buf);
	int name_len, val_len;
	char *s;
	int ret = 1;
	char kv_buf[1024] = { 0 };

	while (p < end) 
	{
		name_len = *p++;

		if (name_len >= 128) 
		{
			name_len = ((name_len & 0x7f) << 24);
			name_len |= (*p++ << 16);
			name_len |= (*p++ << 8);
			name_len |= *p++;
		}

		val_len = *p++;

		if (val_len >= 128) 
		{
			val_len = ((val_len & 0x7f) << 24);
			val_len |= (*p++ << 16);
			val_len |= (*p++ << 8);
			val_len |= *p++;
		}

		if (name_len + val_len < 0 || name_len + val_len > end - p) 
		{
			/* Malformated request */
			ret = 0;
			break;
		}

		if (name_len+1 >= buf_size) 
		{
			buf_size = name_len + 64;
			tmp = (char*)(tmp == buf ? malloc(buf_size): realloc(tmp, buf_size));
		}

		memcpy(tmp, p, name_len);
		tmp[name_len] = 0;
		s = strndup((char*)p + name_len, val_len);

		p += name_len + val_len;
		
		sprintf(kv_buf, "%s=%s", tmp, s);
		req->envp[ req->env_cnt++ ] = strndup(kv_buf, strlen(kv_buf));
		memset(kv_buf, 0, 1024);
	
	//	fprintf(stderr, "FCGI PARAM %s=%s\n", tmp, s);
	}

	if (tmp != buf && tmp != NULL) 
	{
		free(tmp);
	}

	/***	
	len = q - mybuf - sizeof(fcgi_header);
	int len += fcgi_make_header((fcgi_header*)buf, FCGI_GET_VALUES_RESULT, 0, len);
	if (safe_write(req, buf, sizeof(fcgi_header)+len) != (int)sizeof(fcgi_header)+len) 
	{
		req->keep = 0;
		return 0;
	}
	***/

	return ret;
}

static void fcgi_free_var(char **s)
{
	free(*s);
}

static int fcgi_read_request(fcgi_request *req)
{
	fcgi_header hdr;
	int len, padding;
	unsigned char buf[FCGI_MAX_LENGTH+8];

	req->keep = 0;
	req->in_len = 0;
	req->out_hdr = NULL;
	req->out_pos = req->out_buf;
	req->envp = (char **) malloc( sizeof(char *) * 255 );
	req->env_cnt = 0;

	if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) || hdr.version < FCGI_VERSION_1) {
		return 0;
	}

	len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
	padding = hdr.paddingLength;
	
	while (hdr.type == FCGI_STDIN && len == 0) 
	{
		if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) || hdr.version < FCGI_VERSION_1) 
		{
			return 0;
		}

		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;
//		fprintf(stderr, "fcgi req version,type,contentLen is %d %d %d", hdr.version, hdr.type, len); 
		
	}

	if (len + padding > FCGI_MAX_LENGTH) {
		return 0;
	}

	req->id = (hdr.requestIdB1 << 8) + hdr.requestIdB0;
//	fprintf(stderr, "fcgi begin req version,type,id,contentLen is %d %d %d %d", hdr.version, hdr.type, req->id, len); 

	if (hdr.type == FCGI_BEGIN_REQUEST && len == sizeof(fcgi_begin_request)) 
	{
		char *val;

		if (safe_read(req, buf, len+padding) != len+padding) 
		{
			return 0;
		}

		req->keep = (((fcgi_begin_request*)buf)->flags & FCGI_KEEP_CONN);
		switch ((((fcgi_begin_request*)buf)->roleB1 << 8) + ((fcgi_begin_request*)buf)->roleB0) 
		{
			case FCGI_RESPONDER:
				val = strdup("FCGI_ROLE=RESPONDER"); break;
			case FCGI_AUTHORIZER:
				val = strdup("FCGI_ROLE=AUTHORIZER"); break;
			case FCGI_FILTER:
				val = strdup("FCGI_ROLE=FILTER"); break;
			default:
				return 0;
		}

		req->envp[ req->env_cnt++ ] = val;

	//	fprintf(stderr, "fcgi role is %s", val);

		if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) || hdr.version < FCGI_VERSION_1) {
			return 0;
		}

		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;
	//	fprintf(stderr, "fcgi param req version,type,contentLen is %d %d %d", hdr.version, hdr.type, len); 

		while (hdr.type == FCGI_PARAMS && len > 0) 
		{
			if (len + padding > FCGI_MAX_LENGTH) {
				return 0;
			}

			if (safe_read(req, buf, len+padding) != len+padding) {
				req->keep = 0;
				return 0;
			}

			if (!fcgi_get_params(req, buf, buf+len)) {
				req->keep = 0;
				return 0;
			}

			if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) || hdr.version < FCGI_VERSION_1) {
				req->keep = 0;
				return 0;
			}

			len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			padding = hdr.paddingLength;

	//		fprintf(stderr, "fcgi param req version,type,contentLen is %d %d %d", hdr.version, hdr.type, len); 
	
		}
	} 
	else if (hdr.type == FCGI_GET_VALUES) 
	{
		int j;
		unsigned char *p = buf + sizeof(fcgi_header);

		if (safe_read(req, buf, len+padding) != len+padding) {
			req->keep = 0;
			return 0;
		}

		if (!fcgi_get_params(req, buf, buf+len)) {
			req->keep = 0;
			return 0;
		}

		for (j = 0; j < sizeof(fcgi_mgmt_vars) / sizeof(fcgi_mgmt_vars[0]); j++) 
		{
			char ** envp = 	req->envp;
			for ( ; *envp != NULL; envp++) 
			{
				if( strncmp( fcgi_mgmt_vars[j].name, *envp, fcgi_mgmt_vars[j].name_len) == 0 ) 
				{
					sprintf((char*)p, "%c%c%s%c", fcgi_mgmt_vars[j].name_len, 1, fcgi_mgmt_vars[j].name, fcgi_mgmt_vars[j].val);
		        	p += fcgi_mgmt_vars[j].name_len + 3;
				}
			}  
		}

		len = p - buf - sizeof(fcgi_header);
		len += fcgi_make_header((fcgi_header*)buf, FCGI_GET_VALUES_RESULT, 0, len);
		if (safe_write(req, buf, sizeof(fcgi_header)+len) != (int)sizeof(fcgi_header)+len) 
		{
			req->keep = 0;
			return 0;
		}
		return 0;
	} 
	else 
	{
		return 0;
	}

	req->envp[ req->env_cnt ] = NULL;

	return 1;
}

int fcgi_read(fcgi_request *req, char *str, int len)
{
	int ret, n, rest;
	fcgi_header hdr;
	unsigned char buf[255];

	n = 0;
	rest = len;
	while (rest > 0) 
	{
		if (req->in_len == 0) 
		{
			if ( safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) || hdr.version < FCGI_VERSION_1 || hdr.type != FCGI_STDIN ) {
				req->keep = 0;
				return 0;
			}

			req->in_len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			req->in_pad = hdr.paddingLength;
			if (req->in_len == 0) {
				return n;
			}
		}

		if (req->in_len >= rest) 
		{
			ret = safe_read(req, str, rest);
		} 
		else 
		{
			ret = safe_read(req, str, req->in_len);
		}

		if (ret < 0) 
		{
			req->keep = 0;
			return ret;
		} 
		else if (ret > 0) 
		{
			req->in_len -= ret;
			rest -= ret;
			n += ret;
			str += ret;
			if (req->in_len == 0) 
			{
				if (req->in_pad) 
				{
					if (safe_read(req, buf, req->in_pad) != req->in_pad) 
					{
						req->keep = 0;
						return ret;
					}
				}
			} 
			else 
			{
				return n;
			}
		} 
		else 
		{
			return n;
		}
	}
	return n;
}

void fcgi_close(fcgi_request *req, int force, int destroy)
{
	if (destroy) {
		free(req->envp);
		req->env_cnt = 0;
	}

	if ((force || !req->keep) && req->fd >= 0) 
	{
		if (!force) 
		{
			char buf[8];
			shutdown(req->fd, 1);
			while (recv(req->fd, buf, sizeof(buf), 0) > 0) {}
		}
		close(req->fd);
		req->fd = -1;

#if PHP_FASTCGI_PM
		if (fpm) fpm_request_finished();
#endif

	}
}

static inline fcgi_header* open_packet(fcgi_request *req, fcgi_request_type type)
{
	req->out_hdr = (fcgi_header*) req->out_pos;
	req->out_hdr->type = type;
	req->out_pos += sizeof(fcgi_header);
	return req->out_hdr;
}

static inline void close_packet(fcgi_request *req)
{
	if (req->out_hdr) {
		int len = req->out_pos - ((unsigned char*)req->out_hdr + sizeof(fcgi_header));
		req->out_pos += fcgi_make_header(req->out_hdr, (fcgi_request_type)req->out_hdr->type, req->id, len);
		req->out_hdr = NULL;
	}
}

int fcgi_flush(fcgi_request *req, int close)
{
	int len;

	close_packet(req);

	len = req->out_pos - req->out_buf;

	if (close) 
	{
		fcgi_end_request_rec *rec = (fcgi_end_request_rec*)(req->out_pos);

		fcgi_make_header(&rec->hdr, FCGI_END_REQUEST, req->id, sizeof(fcgi_end_request));
		rec->body.appStatusB3 = 0;
		rec->body.appStatusB2 = 0;
		rec->body.appStatusB1 = 0;
		rec->body.appStatusB0 = 0;
		rec->body.protocolStatus = FCGI_REQUEST_COMPLETE;
		len += sizeof(fcgi_end_request_rec);
	}

	if (safe_write(req, req->out_buf, len) != len) {
		req->keep = 0;
		return 0;
	}

	req->out_pos = req->out_buf;
	return 1;
}

int fcgi_write(fcgi_request *req, fcgi_request_type type, const char *str, int len)
{
	int limit, rest;

	if (len <= 0) 
	{
		return 0;
	}

	if (req->out_hdr && req->out_hdr->type != type) 
	{
		close_packet(req);
	}

#if USE_UN_OPTIMIZED

	/* Unoptimized, but clear version */
	rest = len;
	while (rest > 0) 
	{
		limit = sizeof(req->out_buf) - (req->out_pos - req->out_buf);

		if (!req->out_hdr) 
		{
			if (limit < sizeof(fcgi_header)) 
			{
				if (!fcgi_flush(req, 0)) 
				{
					return -1;
				}
			}
			open_packet(req, type);
		}

		limit = sizeof(req->out_buf) - (req->out_pos - req->out_buf);
		if (rest < limit) 
		{
			memcpy(req->out_pos, str, rest);
			req->out_pos += rest;
			return len;
		} 
		else 
		{
			memcpy(req->out_pos, str, limit);
			req->out_pos += limit;
			rest -= limit;
			str += limit;
			if (!fcgi_flush(req, 0)) 
			{
				return -1;
			}
		}
	}

#else

	/* Optimized version */
	limit = sizeof(req->out_buf) - (req->out_pos - req->out_buf);
	if (!req->out_hdr) {
		limit -= sizeof(fcgi_header);
		if (limit < 0) limit = 0;
	}

	if (len < limit) {

		if (!req->out_hdr) {
			open_packet(req, type);
		}

		memcpy(req->out_pos, str, len);
		req->out_pos += len;

	} else if (len - limit < sizeof(req->out_buf) - sizeof(fcgi_header)) {

		if (!req->out_hdr) {
			open_packet(req, type);
		}

		if (limit > 0) {
			memcpy(req->out_pos, str, limit);
			req->out_pos += limit;
		}   

		if (!fcgi_flush(req, 0)) {
			return -1; 
		}   

		if (len > limit) {
			open_packet(req, type);
			memcpy(req->out_pos, str + limit, len - limit);
			req->out_pos += len - limit;
		}   

	} else {

		int pos = 0;
		int pad;

		close_packet(req);
		while ((len - pos) > 0xffff) {

			open_packet(req, type);
			fcgi_make_header(req->out_hdr, type, req->id, 0xfff8);
			req->out_hdr = NULL;
			if (!fcgi_flush(req, 0)) {
				return -1; 
			}   
			if (safe_write(req, str + pos, 0xfff8) != 0xfff8) {
				req->keep = 0;
				return -1; 
			}
			pos += 0xfff8;

		}

		pad = (((len - pos) + 7) & ~7) - (len - pos);
		rest = pad ? 8 - pad : 0;

		open_packet(req, type);
		fcgi_make_header(req->out_hdr, type, req->id, (len - pos) - rest);
		req->out_hdr = NULL;

		if (!fcgi_flush(req, 0)) {
			return -1;
		}

		if (safe_write(req, str + pos, (len - pos) - rest) != (len - pos) - rest) {
			req->keep = 0;
			return -1;
		}

		if (pad) {
			open_packet(req, type);
			memcpy(req->out_pos, str + len - rest,  rest);
			req->out_pos += rest;
		}
	}

#endif

	return len;
}

int fcgi_finish_request(fcgi_request *req)
{
	if (fcgi_is_fastcgi() && req->fd >= 0) 
	{
		fcgi_flush(req, 1);
		fcgi_close(req, 0, 1);
	}
	return 1;
}

char* fcgi_getenv(fcgi_request *req, const char* var, int var_len)
{
	if (!req) return NULL;

	char ** envp = 	req->envp;
	for ( ; *envp != NULL; envp++) {
		
		if( strncmp( var, *envp, var_len) == 0 ) {
			char * p = strchr(*envp, '=');
			return strdup( p + 1 );
		}
	}  
	return NULL;
}

char* fcgi_putenv(fcgi_request *req, char* var, int var_len, char* val)
{
	if (var && req) 
	{
		char kv_buf[1024] = { 0 };
		char ** envp = 	req->envp;
		for ( ; *envp != NULL; envp++) {
			if( strncmp( var, *envp, var_len) == 0 ) {
				sprintf(kv_buf, "%s=%s", var, val);
				*envp = strndup(kv_buf, strlen(kv_buf));
				return *envp ;
			}
		}  
	}    
	return NULL;
}

int fcgi_accept_request(fcgi_request *req)
{
	fcgi_finish_request(req);

	while (1) 
	{
		if (req->fd < 0) 
		{
			while (1) 
			{
				if (in_shutdown) 
				{
					return -1;
				}

				int listen_socket = req->listen_socket;

				//fprintf(stderr, "listening socket is %d\n", listen_socket);
				sa_t sa;
				socklen_t len = sizeof(sa);

#if PHP_FASTCGI_PM
				if (fpm) fpm_request_accepting();
#endif

				FCGI_LOCK(req->listen_socket);
				req->fd = accept(listen_socket, (struct sockaddr *)&sa, &len);
				FCGI_UNLOCK(req->listen_socket);
				
			//	fprintf(stderr, "Connection from IP address '%s' is processing request.\n", inet_ntoa(sa.sa_inet.sin_addr));
				if (req->fd >= 0 && allowed_clients) 
				{
					int n = 0;
					int allowed = 0;

			   		while (allowed_clients[n] != INADDR_NONE) 
					{
			   			if (allowed_clients[n] == sa.sa_inet.sin_addr.s_addr) 
						{
			   				allowed = 1;
			   				break;
			   			}
			    		n++;
			    	}

					if (!allowed) 
					{
						fprintf(stderr, "Connection from disallowed IP address '%s' is dropped.\n", inet_ntoa(sa.sa_inet.sin_addr));
						closesocket(req->fd);
						req->fd = -1;
						continue;
					}
				}

				if (req->fd < 0 && (in_shutdown || (errno != EINTR && errno != ECONNABORTED))) 
				{
					return -1;
				}

				if (req->fd >= 0) 
				{

#if PHP_FASTCGI_PM
					if (fpm) fpm_request_reading_headers();
#endif

#if defined(HAVE_SYS_POLL_H) && defined(HAVE_POLL)
					struct pollfd fds;
					int ret;

					fds.fd = req->fd;
					fds.events = POLLIN;
					fds.revents = 0;
					do 
					{
						errno = 0;
						ret = poll(&fds, 1, 5000);
					}while (ret < 0 && errno == EINTR);
					
					if (ret > 0 && (fds.revents & POLLIN)) 
					{
						break;
					}
					fcgi_close(req, 1, 0);

#else
					if (req->fd < FD_SETSIZE) 
					{
						struct timeval tv = {5,0};
						fd_set set;
						int ret;

						FD_ZERO(&set);
						FD_SET(req->fd, &set);
						do {
							errno = 0;
							ret = select(req->fd + 1, &set, NULL, NULL, &tv) >= 0;
						} while (ret < 0 && errno == EINTR);
					
						if (ret > 0 && FD_ISSET(req->fd, &set)) 
						{
							break;
						}
						fcgi_close(req, 1, 0);
					} 
					else 
					{
						fprintf(stderr, "Too many open file descriptors. FD_SETSIZE limit exceeded.");
						fcgi_close(req, 1, 0);
					}
#endif

				}
			}
		} 
		else if (in_shutdown) 
		{
			return -1;
		}
		
		if (fcgi_read_request(req)) 
		{
			/***
			char ** envp = 	req->envp;
			for ( ; *envp != NULL; envp++) {
		     	fprintf(stderr, "%s\n", *envp); 
			}  
			char * val1 = fcgi_getenv(req, "DOCUMENT_ROOT", strlen("DOCUMENT_ROOT") );
			char * val2 = fcgi_getenv(req, "REQUEST_URI", strlen("REQUEST_URI") );
		     
			fprintf(stderr, "%s\n", val1); 
			fprintf(stderr, "%s\n", val2); 
			***/

			return req->fd;
		} 
		else 
		{
			fcgi_close(req, 1, 1);
		}
	}
}

