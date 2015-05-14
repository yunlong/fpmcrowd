/**
 * @copyright   	Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#define USE_HTTPMSG_H
#define USE_SOCKET_H
#define USE_UTILS_H
#define USE_CROWD_CLIENT_H

#include "dcrowd.h"

namespace dcrowd {

//////////////////////////class TCrowdClient Implement /////////////////////
TCrowdClient :: TCrowdClient()
{
	htRequest = new THttpMsg;
	htResponse = new THttpMsg;
}

TCrowdClient :: ~TCrowdClient(void)
{
	if(htRequest != NULL) delete htRequest;
	if(htResponse != NULL) delete htResponse;
}

int TCrowdClient :: ReadLine(char *line, int maxsize)
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


/* function to Parse the http headers from the socket */
int TCrowdClient :: recvHttpResponse(void)
{
	int ret;
	char *ptr, *strKey, *strVal;
    char linebuf[MAX_LINE_LEN];

	rline_t   rline;
	readLineExInit(getHandle(), linebuf, MAX_LINE_LEN, &rline);

	htResponse->ClearAllHdr();

/****
	ret = ReadLine(buf, LINE_BUFFER);
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
		memset(linebuf, 0, sizeof(linebuf));
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
		
		htResponse->SetHdr(strKey, strVal);
	}
	
//	htResponse->ShowAllHttpHdr();

	return 0;
}

THttpMsg* TCrowdClient::getHttpResponse()
{
	recvHttpResponse();
	return htResponse;
}

void TCrowdClient::buildReqVerbHdr( TCrowdReqVerb verb )
{
	if(htRequest != NULL)
	{	
		string reqverb;
    	switch (verb)
    	{    
	    case VERB_REC_ITEMS			:	reqverb = "REC_ITEMS";			break;
	    case VERB_REC_ITEMS_EX		:	reqverb = "REC_ITEMS_EX";		break;
		case VERB_REC_SEARCH 		:	reqverb = "REC_SEARCH"; 		break;
	    case VERB_REC_SIMILAR_USERS	:	reqverb = "REC_SIMILAR_USERS"; 	break;
    	case VERB_REC_SIMILAR_ITEMS	:	reqverb = "REC_SIMILAR_ITEMS"; 	break;
		case VERB_NONE				:   reqverb = "REC_ITEMS";			break;
    	}    

		htRequest->SetHdr("REQ_VERB", reqverb);
	}
}

void TCrowdClient::buildWebguidHdr( string guid )
{
	if(htRequest != NULL)
	{	
		htRequest->SetHdr("WEBGUID", guid);
	}
}

void TCrowdClient::buildConfPathHdr( string confpath )
{
	if(htRequest != NULL)
	{	
		htRequest->SetHdr("CONFPATH", confpath);
	}
}

void TCrowdClient::buildRSAHdr( string rsa )
{
	if(htRequest != NULL)
	{
		htRequest->SetHdr("RSA", rsa);
	}
}

void TCrowdClient::buildRDMHdr( string rdm )
{
	if(htRequest != NULL)
	{
		htRequest->SetHdr("RDM", rdm);
	}
}

void TCrowdClient::buildUserIdHdr(string userid)
{
	if(htRequest != NULL)
	{	
		htRequest->SetHdr("USERID", userid);
	}
}

void TCrowdClient::buildItemIdHdr(string itemid)
{
	if(htRequest != NULL)
	{	
		htRequest->SetHdr("ITEMID", itemid);
	}
}

void TCrowdClient::buildHowManyHdr( string howmany )
{
	if(htRequest != NULL)
	{	
		htRequest->SetHdr("HOWMANY", howmany);
	}

}

void TCrowdClient::buildSegNumHdr( string segnum )
{
    if(htRequest != NULL)
    {   
        htRequest->SetHdr("SEGNUM", segnum);
    }   
}

void TCrowdClient :: buildSegItemsHdr( string segContent, string segno )
{
    if(htRequest != NULL)
    {   
        htRequest->SetHdr(segContent, segno);
    }   
}

int TCrowdClient::sendHttpRequest( )
{
	char * szBuffer;
	int pos = 0, nHdr;
	THttpHdr * pHdr;
	
	szBuffer = new char[htRequest->GetAllHdrLen() + 3];	
	if(htRequest != NULL && htRequest->GetHdrCount() > 0)
	{
		nHdr = htRequest->GetHdrCount();
    	for(int i = 0; i < nHdr; i++)
		{
			pHdr = htRequest->GetHdr( i );
		
	        snprintf(szBuffer + pos, MAX_LINE_LEN, "%s: %s\r\n", pHdr->GetKey().c_str(), pHdr->GetVal().c_str());
			pos += htRequest->GetHdrLen(i);
    	}
		sprintf(szBuffer + pos, "\r\n");
	}

	int retval = Send(szBuffer, strlen(szBuffer));
	delete szBuffer;
	return retval;

}


void TCrowdClient::handleRead()
{

}

void TCrowdClient::handleWrite()
{

}

void TCrowdClient::handleClose()
{

}

}

