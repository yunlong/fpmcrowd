/**
 * @copyright   		Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef __CROWD_SESSION_H__
#define __CROWD_SESSION_H__

/****************
Key: Value

Crowd Request Format
webguid: 222 \r\n
confpath: /usr/local/crowd/conf/crowd.cn \r\n
rsa:	0 \r\n
userid: 22 \r\n
num	: 20 \r\n
itemid:	33\r\n
optional:
OTHERITEMS: 1,2,3,4,5,6,8\r\n

Crowd Response Format
itemid: prefvalue
...
******************/

namespace dcrowd {

class TCrowdSession : public TcpSocket
{
private :	
	THttpMsg * htRequest;
	THttpMsg * htResponse;
public  :
	
	TCrowdSession(void);
	~TCrowdSession(void);
	size_t GetHttpRequestLen( void ) { return htRequest->GetAllHdrLen(); 	}
	size_t GetHttpResponseLen(void ) { return htResponse->GetAllHdrLen(); 	}
	/////////////////////////////////////////////
	int ReadLine(char *line, int maxsize);
	int recvHttpRequest();
	int sendHttpResponse();
	//////////////////////////////////////////
	
	void handleRead(void);
	void handleWrite(void);
	void handleClose(void);

};

}

#endif
