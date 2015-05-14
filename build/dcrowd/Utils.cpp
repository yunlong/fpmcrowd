/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#define _(string)			gettext(string)
#define N(string)			string

#define USE_UTILS_H
#define USE_SHA_H
#define USE_DBG_H
#include "dcrowd.h"

namespace dcrowd {

void readLineExInit(int fd, char *ptr, size_t maxlen, rline_t *rptr)
{
	rptr->read_fd = fd;             /* save caller's arguments */
	rptr->read_ptr = ptr;
	rptr->read_maxlen = maxlen;

	rptr->rl_cnt = 0;               /* and init our counter & pointer */
	rptr->rl_bufptr = rptr->rl_buf;
}

ssize_t readcEx(rline_t *rptr, char *ptr)
{
	if (rptr->rl_cnt <= 0)
	{
again:
		rptr->rl_cnt = read(rptr->read_fd, rptr->rl_buf, sizeof(rptr->rl_buf));
		if (rptr->rl_cnt < 0)
		{
			if (errno == EINTR)
				goto again;
			else
				return -1;
		}
		else if (rptr->rl_cnt == 0)
			return 0;
		rptr->rl_bufptr = rptr->rl_buf;
	}

	rptr->rl_cnt--;
	*ptr = *rptr->rl_bufptr++ & 255;
	return 1;
}

size_t readLineEx(rline_t *rptr)
{
	int     n, rc;
	char    c, *ptr;

	ptr = rptr->read_ptr;
	for (n = 1; n < rptr->read_maxlen; n++)
	{
		if ( (rc = readcEx(rptr, &c)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
			break;
		}
		else if (rc == 0)
		{
			if (n == 1)
				return(0);  /* EOF, no data read */
			else
				break;      /* EOF, some data was read */
		}
		else
			return(-1); /* error */
	}

	*ptr = 0;
	return n;
}

int safe_open(const char *pathname,  int flags)
{
	int ret;
	ret = open(pathname,  flags, 0777);
	return ret;
}

ssize_t safe_write(int fd,  const void *buf,  size_t count)
{
	ssize_t n;

	do 
	{
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);
	
	return n;
}

ssize_t safe_read(int fd,  void *buf,  size_t count)
{
	ssize_t n;

	do 
	{
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

ssize_t full_write(int fd, const void *buf, size_t len)
{
	ssize_t cc;
    ssize_t total;

    total = 0;

    while (len > 0) 
	{
    	cc = safe_write(fd, buf, len);

        if (cc < 0)
        	return cc;  

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }
	return total;
}

//this will rollover ~ every 49.7 days
inline unsigned int GetTickCount(void)
{
	struct timeval tv;
    gettimeofday( &tv, NULL );
    return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

inline bool createDir(const char* path)
{
	std::string strPath = path;
	std::string::size_type pos = strPath.rfind('/');
	if(pos == std::string::npos)
	{
		return false;
	}
	if(pos != strPath.size()-1)
	{
		strPath.erase(pos+1, strPath.size()-pos-1);
	}
	
	std::string dirPath;
	for(;;)
	{
		std::string::size_type pos = strPath.find('/',1);
		if(pos == std::string::npos)
		{
			break;
		}
		
		dirPath += strPath.substr(0, pos);
		DIR* dir = opendir(dirPath.c_str());
		if(dir == NULL)
		{
			if(errno == ENOENT)
			{
				if(-1 == mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		
		if(dir != NULL)
		{
			closedir(dir);
		}
		
		strPath.erase(0, pos);
	}
	
	return true;
}

inline std::string extractFileName(const char* fullPath)
{
	std::string strPath = fullPath;
	std::string::size_type pos = strPath.rfind('/');
	if(pos == std::string::npos)
	{
		return fullPath;
	}
	if(pos != strPath.size()-1)
	{
		return strPath.substr(pos+1, strPath.size()-pos-1);
	}
	
	return "";
}

inline std::string extractFilePath(const char* fullPath)
{
	std::string strPath = fullPath;
	std::string::size_type pos = strPath.rfind('/');
	if(pos == std::string::npos)
	{
		return "./";
	}
	if(pos != strPath.size()-1)
	{
		return strPath.substr(0, pos+1);
	}
	
	return fullPath;
}

inline std::string second2Str(unsigned int second)
{
	char buf[128];
	
	if(second<60)
	{
		sprintf(buf, "%u sec", second);
		return buf;
	}
	
	if(second>=60 && second<60*60)
	{
		unsigned int min = second / 60;
		unsigned int sec = second % 60;
		sprintf(buf, "%u min %u sec", min, sec);
		return buf;
	}
	
	unsigned int hou = second/(60*60);
	unsigned int min = (second%(60*60))/60;
	unsigned int sec = second % 60;
	sprintf(buf, "%u hou %u min %u sec", hou, min, sec);
	return buf;	
}

inline std::string byteCount2Str(int64_t count)
{
	char buf[128];
	
	if(count < 1024)
	{
		sprintf(buf, "%d B", (int)count);
		return buf;
	}
	
	if(count>=1024 && count<1024*1024)
	{
		double fcount = ((double)count)/1024.0f;
		sprintf(buf, "%01.2f KB", fcount);
		return buf;
	}
	
	if(count>=1024*1024 && count<1024*1024*1024)
	{
		double fcount = ((double)count)/(1024.0f*1024.0f);
		sprintf(buf, "%01.2f MB", fcount);
		return buf;		
	}
	
	double fcount = ((double)count)/(1024.0f*1024.0f*1024.0f);
	sprintf(buf, "%01.2f GB", fcount);
	return buf;		
}

inline int64_t htonll(int64_t number)
{
	if(htons(1) == 1)
	{
		return number;
	}
	
	return ( htonl( (number >> 32) & 0xFFFFFFFF) |
		 ((int64_t) (htonl(number & 0xFFFFFFFF))  << 32));
}

inline int64_t ntohll(int64_t number)
{
	if(htons(1) == 1)
	{
		return number;
	}
		
 	return ( htonl( (number >> 32) & 0xFFFFFFFF) |
		((int64_t) (htonl(number & 0xFFFFFFFF))  << 32));
}

inline std::string getPeerStr(const char* ip, unsigned short port)
{
	char buf[128];
	sprintf(buf, "%s:%u", ip, port);
	return buf;
}


/****************************************************
 *  MD5  Base64
 ******************************************************/
// Unsafe character map for the range 0x20..0x7F according to RFC1738
static BYTE s_byUrlEscapeMap[0x80-0x20] =
{
//    ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
    1,0,1,1,0,1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,

//  @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \ ] ^ _
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,

//  ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ DEL
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1
};

string UrlEncode( CPCHAR sz )
{
    CPBYTE pSrc;
    UINT nLength = 0;

    // Find string length and count of escaped chars
    for( pSrc = (CPBYTE)sz; *pSrc; pSrc++ )
    {
        nLength++;
        if( *pSrc >= 0x20 && *pSrc < 0x80 ) nLength += 2*s_byUrlEscapeMap[ *pSrc - 0x20 ];
    }

    string strEncoded( nLength, '\0' );
    PBYTE pDest = (PBYTE)(CPBYTE)strEncoded.c_str(); // Do this with operator[] instead?
    for( pSrc = (CPBYTE)sz; *pSrc; pSrc++ )
    {
        if( *pSrc >= 0x20 && *pSrc < 0x80 && s_byUrlEscapeMap[ *pSrc - 0x20 ] )
        {
            char chHex1 = '0' + ( *pSrc / 16 );
            if( chHex1 > '9' ) chHex1 += ( 'A' - ('9'+1) );
            char chHex2 = '0' + ( *pSrc % 16 );
            if( chHex2 > '9' ) chHex2 += ( 'A' - ('9'+1) );
            *pDest++ = '%';
            *pDest++ = chHex1;
            *pDest++ = chHex2;
        }
        else
        {
            *pDest++ = *pSrc;
        }
    }

    return strEncoded;
}

std::string UrlDecode( CPCHAR sz )
{
    CPBYTE pSrc;
    UINT nLength = 0;

    for( pSrc = (CPBYTE)sz; *pSrc; pSrc++ )
    {
        nLength++;
        if( '%' == *pSrc &&
            isxdigit( *(pSrc+1) ) &&
            isxdigit( *(pSrc+2) ) )
        {
            pSrc += 2;
        }
    }

    string strDecoded( nLength, '\0' );
    PBYTE pDest = (PBYTE)(CPBYTE)strDecoded.c_str();
    for( pSrc = (CPBYTE)sz; *pSrc; pSrc++ )
    {
        if( '%' == *pSrc &&
            isxdigit( *(pSrc+1) ) &&
            isxdigit( *(pSrc+2) ) )
        {
            BYTE byHex1 = toupper( *(pSrc+1) ) - '0';
            if( byHex1 > 9 ) byHex1 -= ( 'A' - ('9'+1) );
            BYTE byHex2 = toupper( *(pSrc+2) ) - '0';
            if( byHex2 > 9 ) byHex2 -= ( 'A' - ('9'+1) );
            *pDest++ = byHex1 * 16 + byHex2;
            pSrc += 2;
        }
        else
        {
            *pDest++ = *pSrc;
        }
    }

    return strDecoded;
}

std::string Base64Encode( CPCHAR sz )
{
    UINT nLen = strlen( sz );
    if( !nLen )
    {
        fprintf(stdout,"Base64Encode: empty string" );
        return "";
    }

    string strEncoded( 'x', 4*((nLen+2)/3) );
    PCHAR pDest = (PCHAR)strEncoded.c_str();
    while( *sz )
    {
        int quad = 0;
        int n;
        for( n = 0; n < 3; n++ )
        {
            quad <<= 8;
            if( *sz )
            {
                quad += (int)(*sz);
                sz++;
            }
        }
        for( n = 3; n >= 0; n-- )
        {
            if( quad & 0xFF )
            {
                int val = quad & 0x3F;
                if( val < 26 )       *(pDest+n) = 'A' -  0 + val;
                else if( val <  52 ) *(pDest+n) = 'a' - 26 + val;
                else if( val <  62 ) *(pDest+n) = '0' - 52 + val;
                else if( val == 62 ) *(pDest+n) = '+';
                else *(pDest+n) = '/';
            }
            else
            {
                *(pDest+n) = '=';
            }
            quad >>= 6;
        }
        pDest += 4;
    }

    return strEncoded;
}

std::string Base64Decode( CPCHAR sz )
{
    UINT nLen = strlen( sz );
    if( !nLen || nLen % 4 )
    {
        fprintf(stdout, "Base64Decode: length %u for '%s'", nLen, sz );
        return "";
    }

    string strDecoded( 3*((nLen+3)/4), '\0' );
    PCHAR pDest = (PCHAR)strDecoded.c_str();
    while( *sz )
    {
        int trip = 0;
        int n;
        for( n = 0; n < 4; n++ )
        {
            int val = 0;
            if( *sz >= 'A' && *sz <= 'Z' )      val = *sz - 'A' + 0;
            else if( *sz >= 'a' && *sz <= 'z' ) val = *sz - 'a' + 26;
            else if( *sz >= '0' && *sz <= '9' ) val = *sz - '0' + 52;
            else if( *sz == '+' ) val = 62;
            else if( *sz == '/' ) val = 63;
            trip <<= 6;
            trip += val;
            sz++;
        }
        for( n = 2; n >= 0; n-- )
        {
            *(pDest+n) = trip & 0xFF;
            trip >>= 8;
        }
        pDest += 3;
    }

    return strDecoded;
}

size_t StrLen(const char *str) 
{
	int len = 0;
	while(str[len++]);
	return len - 1;
}

int StrCmp(const char * s1, const char * s2)
{
	int retval;
	int i = 0;
	while((retval = s1[i] - s2[i]) == 0)
	{	
		if(s1[i] == 0) break;
	    i++;	
	}
	return retval;
}

char * StrCpy(char * dst, const char * src)
{
	char * t = dst;
	while((*t++ = *src++) != 0);
	return dst;
}

char * StrnCpy(char * dst, const char * src, size_t n)
{
	char * t = dst;
	while(n > 0)
	{
		*t++ = *src++;
		n--;
	}
	return dst;
}


size_t get_nl(char *sfrom) 
{
	unsigned char *from = (unsigned char *)sfrom;
	size_t t;
	t = (*from++) << 24;
	t |= (*from++) << 16;
	t |= (*from++) << 8;
	t |= *from;
	return t;
}

void set_nl(char *sto, size_t from)
{
	unsigned char *to = (unsigned char *)sto;
	*to++ = (from >> 24) & 0xff; 
	*to++ = (from >> 16) & 0xff; 
	*to++ = (from >> 8) & 0xff; 
	*to = from & 0xff; 
}

void htons_buf( UINT16* pnbuf, const void* phbuf, UINT cnt )
{
    register UINT16 tmp;
    while( cnt )
    {
        memcpy( &tmp, phbuf, 2 );
        *pnbuf = htons( tmp );
        phbuf = (char*)phbuf + 2; pnbuf++; cnt--;
    }
}

void ntohs_buf( UINT16* phbuf, const void* pnbuf, UINT cnt )
{
    register UINT16 tmp;
    while( cnt )
    {
        memcpy( &tmp, pnbuf, 2 );
        *phbuf = ntohs( tmp );
        pnbuf = (char*)pnbuf + 2; phbuf++; cnt--;
    }
}

void htonl_buf( UINT32* pnbuf, const void* phbuf, UINT cnt )
{
    register UINT32 tmp;
    while( cnt )
    {
        memcpy( &tmp, phbuf, 4 );
        *pnbuf = htonl( tmp );
        phbuf = (char*)phbuf + 4; pnbuf++; cnt--;
    }
}

void ntohl_buf( UINT32* phbuf, const void* pnbuf, UINT cnt )
{
    register UINT32 tmp;
    while( cnt )
    {
        memcpy( &tmp, pnbuf, 4 );
        *phbuf = ntohl( tmp );
        pnbuf = (char*)pnbuf + 4; phbuf++; cnt--;
    }
}


int StrToInt(const char * s)
{
	int i = 0;
	while(*s != '\0')
		i = i * 10 + (*s++ - '0');
	return i;
}

double Pow(double x, int y)
{
	int i;
	double sum = 1;

	for(i = 1; i <= y; i++)
		sum *= x;
    return sum;
}

char * itoa(int n, char* str) 
{
	char s[25] = { 0 };
	char *cp = s + sizeof(s) - 1;
	unsigned long m;
	if (n == LONG_MIN)
		m = (unsigned long)LONG_MAX + 1;
	else if (n < 0)
		m = -n;
	else
		m = n;
	
	while(m > 0)
	{	
		*--cp = m % 10 + '0'; 	
		m /= 10;
	}

	if (n < 0)
		*--cp = '-';
	
	if(n == 0)
		*--cp = '0';

	char * t = str;
	while((*t++ = *cp++) != 0);
	return str;
}

char * ftoa(double num, char * str, int ndec)
{
	int t;
	int Dec;
	char ch[25] = { 0 };
	char * s = ch + 23; 

	int zerocnt = 0;

	if(num < 0)  
	{   
		t = -(int)num ;
		
		double  x = -num - t;
		if(x > 0 )
		{
			while( (x *= 10) < 1 ) { zerocnt++; }	
		}

		Dec =(int)( (-num - t) * Pow(10, ndec) );   
	}   
	else
	{   
		t = (int)num;
		
		double  x = num - t;
		if(x > 0 )
		{
			while( (x *= 10) < 1 ) { zerocnt++; }	
		}

		Dec = (int)( (num - t) * Pow(10, ndec) );    
	}   
	
	while(Dec > 0)
	{   
		*s-- = Dec % 10 + '0';
		Dec /= 10; 
	}   
	
	while(zerocnt-- > 0)
		*s-- = '0';

	*s-- = '.';
	
	if(t == 0) 	*s-- = '0';

	while(t > 0)
	{   
		*s-- = t % 10 + '0';
		t /= 10; 
	}   

	if(num < 0) *s-- = '-';

	char * cp = str;
	while((*cp++ = *++s) != 0);
	return str;
}

void showMsg(const char * args, ...)
{
    char p[1024];
    va_list vp;
    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);
    fprintf(stdout, "%s\n", p);
}

int  createOutFile(const char * PathName )
{
    int oflags = O_WRONLY | O_CREAT | O_TRUNC;
    int iflags = O_RDONLY;
    size_t out_full = 0;
    size_t out_part = 0;
    size_t in_full = 0;
    size_t in_part = 0;
    size_t count = 500000;
    size_t bs = 8192;
    ssize_t n;
    int sync_flag = 0;
    int trunc_flag = 1;
    int ifd;
    int ofd;
    char *buf;

    if((ifd = safe_open("/dev/zero",  iflags)) < 0)
        return -1;

    if((ofd = safe_open(PathName, oflags)) < 0 )
        return -1;

    buf = (char *)malloc(bs);

    while (in_full + in_part != count)
    {
        memset(buf, '\0', bs);
        n = safe_read(ifd, buf, bs);
        if (n < 0)  return -1;
        if (n == 0) break;

        if (n == bs)
            in_full++;
        else
            in_part++;

        if (sync_flag)
        {
            memset(buf + n, '\0', bs - n);
            n = bs;
        }
        n = full_write(ofd, buf, n);
        if (n < 0) return -1;

        if (n == bs)
            out_full++;
        else
            out_part++;
    }

    if (close (ifd) < 0)
        return -1;
    if (close (ofd) < 0)
        return -1;

    fprintf(stderr, "%ld+%ld records in\n%ld+%ld records out\n",
                                    (long)in_full, (long)in_part,
                                    (long)out_full, (long)out_part);

    return 0;
}



int is_number(char *str)
{
    int i = 0;

    while (str[i])
    {
    if (isdigit(str[i]) == 0)
    {
        return 0;
    }
    i++;
    }
    return 1;
}

bool IsFileExist(const char * FileName)
{
    struct stat stbuf;
    if(stat(FileName, &stbuf) == -1)
        return false;
    else
        return true;
}


/* How many digits in a (long) integer? */
int numdigit(long a)
{
    int res;

    for (res = 1; (a /= 10) != 0; res++);
    return res;
}

int diffTime(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->tv_usec < y->tv_usec)
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }

    if (x->tv_usec - y->tv_usec > 1000000)
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    return x->tv_sec < y->tv_sec;
}


void delay_ms(int ms)
{
    struct timeval tv_delay;

    memset(&tv_delay, 0, sizeof(tv_delay));

    tv_delay.tv_sec = ms / 1000;
    tv_delay.tv_usec = (ms * 1000) % 1000000;

    if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv_delay) < 0)
        printf("Warning Unable to delay\n");
}

void TimeToStr(char *timeStr, double time)
{
    long sec, min, hour, day;
    
    min = (long)time / 60; // min
    sec = (long)time % 60; // sec

    if(min < 60) 
    {   
        sprintf(timeStr, "%02d:%02d", min, sec);
        return;
    }   

    hour = min / 60; 
    min %= 60; 

    if(hour < 24) 
    {   
        sprintf(timeStr, "%2dh%2d", hour, min);
        return;
    }   

    day = hour / 24; 
    hour %= 24; 

    if(day < 100)
    {   
        sprintf(timeStr, "%2dd%2d", day, hour);
        return;
    }   
    
    sprintf(timeStr, "--:--");
}

void SizeToStr(char *sizeStr, off_t size)
{
    double dsize = size;

    if(dsize < 0)
    {
        sprintf(sizeStr, "%3ldB", 0);
        return;
    }

    if(dsize < 1000)
    {
        sprintf(sizeStr, "%3ldB", (long)dsize);
        return;
    }

    dsize /= 1024;
    if(dsize < 1000)
    {
        if(dsize <= 9.9)
        {
            sprintf(sizeStr, "%.1fK", dsize);
        }
        else
        {
            sprintf(sizeStr, "%3ldK", (long)dsize);
        }
        return;
    }

    dsize /= 1024;
    if(dsize < 1000)
    {
        if(dsize <= 9.9)
        {
            sprintf(sizeStr, "%.1fM", dsize);
        }
        else
        {
            sprintf(sizeStr, "%3ldM", (long)dsize);
        }
        return;
    }
    dsize /= 1024;
    if(dsize < 1000)
    {
        if(dsize <= 9.9)
        {
            sprintf(sizeStr, "%.1fG", dsize);
        }
        else
        {
            sprintf(sizeStr, "%3ldG", (long)dsize);
        }
        return;
    }
}

/*
 * Convert a string of format "YYYY/MM/DD HH:MM:SS" to time_t
 *  <pubDate>08 Jun 2008 08:29:43</pubDate>
 */
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//Sun, 08 Jun 2008 08:29:43 +0000
time_t transTime(string timestr)
{
    int pos1 = timestr.find_first_of(',') + 2;
    int pos2 = timestr.find_first_of("+") - 2;
    string datestr = timestr.substr(pos1, pos2 - pos1 + 1); 

//  cout << datestr << endl;
    struct tm thisTime;
    thisTime.tm_mday = atoi( datestr.substr(0, 2).c_str() );
    for( int i = 0; i < sizeof(months); i++)
    {   
        if( strcmp(datestr.substr(3, 3).c_str(), months[i]) == 0 ) 
        {
            thisTime.tm_mon = i;
            break;
        }
    }   

    thisTime.tm_year = atoi( datestr.substr(7, 4).c_str() );
    if (thisTime.tm_year > 1900)
        thisTime.tm_year -= 1900;
    
    thisTime.tm_hour = atoi( datestr.substr(12, 2).c_str() );// + 8;  //???
    thisTime.tm_min = atoi( datestr.substr(15, 2).c_str() );
    thisTime.tm_sec = atoi( datestr.substr(18, 2).c_str() );
    thisTime.tm_wday = thisTime.tm_yday = 0;
    thisTime.tm_isdst = -1; 
	/*
    cout << datestr.substr(0, 2) << endl; 
    cout << datestr.substr(3, 3) << endl; 
    cout << datestr.substr(7, 4) << endl; 
    cout << datestr.substr(12, 2) << endl; 
    cout << datestr.substr(15, 2) << endl; 
    cout << datestr.substr(18, 2) << endl; 
    cout << thisTime.tm_year << endl; 
    cout << thisTime.tm_mon << endl; 
    cout << thisTime.tm_mday << endl; 
    cout << thisTime.tm_hour << endl; 
    cout << thisTime.tm_min << endl; 
    cout << thisTime.tm_sec << endl;
*/

    return (mktime(&thisTime));

}

time_t strToTime(char* str)
{
    struct tm thisTime;
    char* temp = strdup(str);

    thisTime.tm_year = atoi(strtok(temp,"/"));
    if (thisTime.tm_year > 1900)
        thisTime.tm_year -= 1900;
    
    thisTime.tm_mon = atoi(strtok(NULL, "/")) - 1;
    thisTime.tm_mday = atoi(strtok(NULL, " "));
    thisTime.tm_hour = atoi(strtok(NULL, ":"));
    thisTime.tm_min = atoi(strtok(NULL, ":"));
    thisTime.tm_sec = atoi(strtok(NULL, ""));
    thisTime.tm_wday = thisTime.tm_yday = 0;
    thisTime.tm_isdst = -1; 
    
    free(temp);
    return (mktime(&thisTime));
}



/***
char * itoa(int Num, char * str)
{
	int i, j = 0;
	char ch[20];
	while(Num > 0)
	{
		ch[j++] = Num % 10 + '0';
		Num /= 10;
	}
	ch[j] = '\0';	
	
	for(i = strlen(ch) - 1, j = 0; i >= 0; i--, j++)
			str[j] = ch[i];
	str[j] = 0;
	
	return str;	
}

char * ftoa(double Num, char * str, int ndec)
{
	int i, j = 0;
	int t = (int)Num;
	int Dec = (int)( (Num - t) * pow(10, ndec) );
	char ch[20] = { 0 };
	char chx[20] = { 0 };
	
	while(t > 0)
	{
		ch[j++] = t % 10 + '0';
		t /= 10;
	}

	for(i = strlen(ch) - 1, j = 0; i >= 0; i--, j++)
			str[j] = ch[i];

	str[j++] = '.';

	i = 0;
	
	while(Dec > 0)
	{
		chx[i++] = Dec % 10 + '0';
		Dec /= 10;
	}
	
	for(i = strlen(chx) - 1; i >= 0; i--, j++)
			str[j] = chx[i];
	str[j] = 0;
	return str;
}
***/



}

