/**
 * @copyright   		Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef __CROWD_CLIENT_H__
#define __CROWD_CLIENT_H__


namespace dcrowd {

typedef enum  {
    VERB_REC_ITEMS,
    VERB_REC_ITEMS_EX,
    VERB_REC_SEARCH,
    VERB_REC_SIMILAR_USERS,
    VERB_REC_SIMILAR_ITEMS,
    VERB_NONE
} TCrowdReqVerb;

class TCrowdClient : public TcpSocket
{
private :	
	THttpMsg * htRequest;
	THttpMsg * htResponse;
public  :
	
	TCrowdClient();
	~TCrowdClient(void);
	size_t GetHttpRequestLen( void ) { return htRequest->GetAllHdrLen(); 	}
	size_t GetHttpResponseLen(void ) { return htResponse->GetAllHdrLen(); 	}
///////////////////////////////////////////////////////////////////
	
	int ReadLine(char *line, int maxsize);
	int recvHttpResponse(void);
	int sendHttpRequest(void);
	THttpMsg* getHttpResponse();

	//////////////////////////////////////////
	void buildReqVerbHdr( TCrowdReqVerb verb );
	void buildWebguidHdr( string guid );
	void buildConfPathHdr( string confpath );
	void buildRSAHdr( string rsa );
	void buildRDMHdr( string rdm );
	void buildUserIdHdr(string userid);
	void buildItemIdHdr(string itemid);
	void buildHowManyHdr( string howmany );
	///////////////////////////////////////////////
    void buildSegNumHdr( string segnum );
    void buildSegItemsHdr( string segContent, string segno );
//	void buildOtherItemsHdr( string otherItems );
 
	
	void handleRead(void);
	void handleWrite(void);
	void handleClose(void);

};

}

#endif
