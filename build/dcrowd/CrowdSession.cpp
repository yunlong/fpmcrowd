/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang 
 * @version          1.0beta
 */
//////////////////////////////////////////////////////////////

#define USE_HTTPMSG_H
#define USE_SOCKET_H
#define USE_UTILS_H
#define USE_CROWD_SESSION_H
#define USE_UTILS_H
#define USE_LOGGER_H
#include "dcrowd.h"
using namespace dcrowd;

#define USE_FILE_RATING_DATAMODEL
#define USE_DBF_RATING_DATAMODEL
#define USE_CACHING_RECOMMENDER


namespace dcrowd {

//////////////////////////class TCrowdSession Implement /////////////////////
TCrowdSession :: TCrowdSession() : TcpSocket()
{
	htRequest = new THttpMsg;
	htResponse = new THttpMsg;
}

TCrowdSession :: ~TCrowdSession(void)
{
	if(htRequest != NULL) delete htRequest;
	if(htResponse != NULL) delete htResponse;
}

int TCrowdSession :: ReadLine(char *line, int maxsize)
{
	int i, retval;
	char c;
	char *ptr;
    
	ptr = line;
	
	for(i = 1; i < maxsize; i++)
    {
		retval = read(getHandle(), &c, 1);
		
		if (retval < 0)
			return retval;	//HERR

		if (retval == 0)
		{
			if(i == 1)
				return 0; //HEOF
			else
				break;
		}

		*ptr = c;
		ptr++;
		if(c == '\n')
			break;
    }

	*ptr = '\0';

	if(i == maxsize)
		return i - 1;
	else
		return i;
}

int TCrowdSession :: recvHttpRequest()
{
	int ret;
	char *ptr, *strKey, *strVal;
	char linebuf[MAX_LINE_LEN];

	rline_t   rline;
	readLineExInit(getHandle(), linebuf, MAX_LINE_LEN, &rline);

	htRequest->ClearAllHdr();

/****
	ret = ReadLine(buf, MAX_LINE_LEN);
	if(ret < 0) return ret;
	if(ret == 0) return -1;
	
	fprintf(stdout, "%s\n", buf);

	ptr = buf;
	while(*ptr == ' ') ptr++;
	
	// skip the http version info
	
	while(*ptr != '\0' && *ptr != ' ' && *ptr != '\r' && *ptr != '\n') ptr++;
	if(*ptr != ' ') return -1;
	while(*ptr == ' ') ptr++;
	StatusCode = atoi(ptr);
****/

	while(1)
	{
		/***
		memset(buf, 0, sizeof(buf));
		ret = ReadLine(buf, MAX_LINE_LEN);
		***/

		ret = readLineEx(&rline);
		if(ret < 0) return ret;
		if(ret == 0) return -1;
		
		ptr = linebuf;
		while(*ptr == ' ') ptr++;
		
		strKey = ptr;
		
		if(*ptr == '\r' || *ptr == '\n') break;
		
		while(*ptr != '\0' && *ptr != ':' && *ptr != '\r' && *ptr != '\n') ptr++;
		
		if(*ptr == ':')
		{
			*ptr = '\0';
			ptr++;
		}
		else
		{
			return -1;
		}

		while(*ptr == ' ') ptr++;
		
		strVal = ptr;
				
		while(*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
		
		*ptr = '\0';
		
		htRequest->SetHdr(strKey, strVal);
	}
	
	LOG(1)("recv crowdProto Request\n");
	if(htRequest != NULL && htRequest->GetHdrCount() > 0)
	{
		int nHdr = htRequest->GetHdrCount();
		THttpHdr * pHdr;
    	for(int i = 0; i < nHdr; i++)
		{
			pHdr = htRequest->GetHdr( i );
	        LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
    	}
	}
	return 0;
}

int TCrowdSession :: sendHttpResponse()
{
	string reqverb  = htRequest->GetHdr("REQ_VERB");

	if(reqverb == "REC_SIMILAR_ITEMS") 
	{
		string webguid  = htRequest->GetHdr("WEBGUID");
		string confpath = htRequest->GetHdr("CONFPATH");
		string rsa 		= htRequest->GetHdr("RSA");
		string rdm 		= htRequest->GetHdr("RDM");
		string itemid   = htRequest->GetHdr("ITEMID");
		string howmany  = htRequest->GetHdr("HOWMANY");
	
		char *strKey, *strVal;
		char kbuf[25], vbuf[25];
		htResponse->ClearAllHdr();
	

		//////////////////////////////////////////////////////////////////////////////////
		char * szBuffer;
		int pos = 0, nHdr;
		THttpHdr * pHdr;
	
		LOG(1)("send crowdProto Response for item%s\n", itemid.c_str() );
		szBuffer = new char[htResponse->GetAllHdrLen() + 3];	
		if(htResponse != NULL && htResponse->GetHdrCount() > 0)
		{
			nHdr = htResponse->GetHdrCount();
	   	 	for(int i = 0; i < nHdr; i++)
			{
				pHdr = htResponse->GetHdr( i );
		
		        LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());

		        snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
				pos += htResponse->GetHdrLen(i);
    		}
			sprintf(szBuffer + pos, "\r\n");
		}
	
		int retval = Send(szBuffer, strlen(szBuffer));
		delete szBuffer;
		return retval;

	}

	if(reqverb == "REC_SIMILAR_USERS") 
	{
		string webguid  = htRequest->GetHdr("WEBGUID");
		string confpath = htRequest->GetHdr("CONFPATH");
		string rsa 		= htRequest->GetHdr("RSA");
		string rdm 		= htRequest->GetHdr("RDM");
		string userid   = htRequest->GetHdr("USERID");
		string howmany  = htRequest->GetHdr("HOWMANY");
		

		char *strKey, *strVal;
		char kbuf[25], vbuf[25];
		htResponse->ClearAllHdr();

		char * szBuffer;
		int pos = 0, nHdr;
		THttpHdr * pHdr;
	
		LOG(1)("send crowdProto Response for user%s\n", userid.c_str() );
		szBuffer = new char[htResponse->GetAllHdrLen() + 3];	
		if(htResponse != NULL && htResponse->GetHdrCount() > 0)
		{
			nHdr = htResponse->GetHdrCount();
	   	 	for(int i = 0; i < nHdr; i++)
			{
				pHdr = htResponse->GetHdr( i );
		
		        LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());

		        snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
				pos += htResponse->GetHdrLen(i);
    		}
			sprintf(szBuffer + pos, "\r\n");
		}
	
		int retval = Send(szBuffer, strlen(szBuffer));
		delete szBuffer;
		return retval;
	}

	if(reqverb == "REC_ITEMS")
	{
		string webguid  = htRequest->GetHdr("WEBGUID");
		string confpath = htRequest->GetHdr("CONFPATH");
		string rsa 		= htRequest->GetHdr("RSA");
		string rdm 		= htRequest->GetHdr("RDM");
		string userid   = htRequest->GetHdr("USERID");
		string howmany  = htRequest->GetHdr("HOWMANY");

		char *strKey, *strVal;
		char kbuf[25], vbuf[25];
		htResponse->ClearAllHdr();

        char * szBuffer;
        int pos = 0, nHdr;
        THttpHdr * pHdr;

        LOG(1)("send crowdProto Response for User%s\n", userid.c_str() );
        szBuffer = new char[htResponse->GetAllHdrLen() + 3];    
        if(htResponse != NULL && htResponse->GetHdrCount() > 0)
        {
            nHdr = htResponse->GetHdrCount();
            for(int i = 0; i < nHdr; i++)
            {
                pHdr = htResponse->GetHdr( i );
    
                LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());

                snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
                pos += htResponse->GetHdrLen(i);
            }
            sprintf(szBuffer + pos, "\r\n");
        }
		
        int retval = Send(szBuffer, strlen(szBuffer));
        delete szBuffer;
        return retval;

		//////////////////////////////////////////////////////////////////////////////////
	}

	if(reqverb == "REC_ITEMS_EX")
	{
		string webguid  = htRequest->GetHdr("WEBGUID");
		string confpath = htRequest->GetHdr("CONFPATH");
		string rsa 		= htRequest->GetHdr("RSA");
		string rdm 		= htRequest->GetHdr("RDM");
		string userid   = htRequest->GetHdr("USERID");
		string howmany  = htRequest->GetHdr("HOWMANY");

		int segnum = atoi( htRequest->GetHdr("SEGNUM").c_str() );

	// 	string format OTHERITEMS : "1,2,3,224,5,2346,237";
        string ItemIds;
        char sbuf[25];
        for( int j = 0; j < segnum; j++ )
        {
            string key = "SEG";
            ItemIds.append( htRequest->GetHdr(key + itoa(j, sbuf)) );
        }

        LOG(1)("send crowdProto Response for User%s\n", userid.c_str() );
		std::list<unsigned int> allOtherItems;
    	if( ItemIds.size() != 0 ) 
    	{   
			LOG(1)("%s\n", ItemIds.c_str() );
        	int pos1 = 0;
	        int pos2 = ItemIds.find_first_of(','); 
    	    while (pos2 != string::npos)
      	  	{
            	string itemid = ItemIds.substr(pos1, pos2 - pos1);
	            pos1 = pos2 + 1 ; 
    	        pos2 = ItemIds.find_first_of(',', pos1 + 1); 
				allOtherItems.push_back( atoi(itemid.c_str()) );
        	}
		
			allOtherItems.push_back( atoi( ItemIds.substr(pos1).c_str() ) );
    	}

		allOtherItems.sort();

	
		char *strKey, *strVal;
		char kbuf[25], vbuf[25];
		htResponse->ClearAllHdr();

        char * szBuffer;
        int pos = 0, nHdr;
        THttpHdr * pHdr;

        LOG(1)("send crowdProto Response for User%s\n", userid.c_str() );
        szBuffer = new char[htResponse->GetAllHdrLen() + 3];    
        if(htResponse != NULL && htResponse->GetHdrCount() > 0)
        {
            nHdr = htResponse->GetHdrCount();
            for(int i = 0; i < nHdr; i++)
            {
                pHdr = htResponse->GetHdr( i );
    
                LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());

                snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
                pos += htResponse->GetHdrLen(i);
            }
            sprintf(szBuffer + pos, "\r\n");
        }
		
        int retval = Send(szBuffer, strlen(szBuffer));
        delete szBuffer;
        return retval;
		//////////////////////////////////////////////////////////////////////////////////
	}

	if(reqverb == "REC_SEARCH")
	{
		string webguid  = htRequest->GetHdr("WEBGUID");
		string confpath = htRequest->GetHdr("CONFPATH");
		string rsa 		= htRequest->GetHdr("RSA");
		string rdm 		= htRequest->GetHdr("RDM");
		string userid   = htRequest->GetHdr("USERID");
		string howmany  = htRequest->GetHdr("HOWMANY");

		int segnum = atoi( htRequest->GetHdr("SEGNUM").c_str() );

	// 	string format OTHERITEMS : "1,2,3,224,5,2346,237";
        string ItemIds;
        char sbuf[25];
        for( int j = 0; j < segnum; j++ )
        {
            string key = "SEG";
            ItemIds.append( htRequest->GetHdr(key + itoa(j, sbuf)) );
        }

		LOG(1)("Search Recommend Other Items for User%s\n", userid.c_str() );
		std::list<unsigned int> allOtherItems;
    	if( ItemIds.size() != 0 ) 
    	{   
			LOG(1)("%s\n", ItemIds.c_str() );
        	int pos1 = 0;
	        int pos2 = ItemIds.find_first_of(','); 
    	    while (pos2 != string::npos)
      	  	{
            	string itemid = ItemIds.substr(pos1, pos2 - pos1);
	            pos1 = pos2 + 1 ; 
    	        pos2 = ItemIds.find_first_of(',', pos1 + 1); 
				allOtherItems.push_back( atoi(itemid.c_str()) );
        	}
		
			allOtherItems.push_back( atoi( ItemIds.substr(pos1).c_str() ) );
    	}

		allOtherItems.sort();
		   
		char *strKey, *strVal;
		char kbuf[25], vbuf[25];
		htResponse->ClearAllHdr();

	//////////////////////////////////////////////////////////////////////////////////
		char * szBuffer;
		int pos = 0, nHdr;
		THttpHdr * pHdr;

		LOG(1)("send crowdProto Response for User%s\n", userid.c_str() );
		szBuffer = new char[htResponse->GetAllHdrLen() + 3];	
		if(htResponse != NULL && htResponse->GetHdrCount() > 0)
		{
			nHdr = htResponse->GetHdrCount();
	    	for(int i = 0; i < nHdr; i++)
			{
				pHdr = htResponse->GetHdr( i );
		
	        	LOG(1)("%s: %s\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());

		        snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
				pos += htResponse->GetHdrLen(i);
    		}
			sprintf(szBuffer + pos, "\r\n");
		}
	
		int retval = Send(szBuffer, strlen(szBuffer));
		delete szBuffer;
		return retval;
	}

	return -1;
}

void TCrowdSession::handleRead()
{
	recvHttpRequest();
}

void TCrowdSession::handleWrite()
{
	sendHttpResponse();
}

void TCrowdSession::handleClose()
{

}

}


