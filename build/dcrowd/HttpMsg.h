/**
 * @copyright   		Copyright (C) 2007, 2008  
 * @author           YunLong.Lee	<yunlong.lee@163.com> XuGang.Wang wangxg@csip.org.cn
 * @version          1.0beta
 */

#ifndef __HTTP_MSG_H__
#define __HTTP_MSG_H__

namespace dcrowd {

class THttpHdr
{
private:
	string m_strKey;
	string m_strVal;
public:
	THttpHdr( void );
	THttpHdr( const string& strKey );
	THttpHdr( const string& strKey, const string& strVal );

	const string&  GetKey( void ) const;
	const string&  GetVal( void ) const;
	void  SetVal( const string& strVal );

};

typedef std::list<THttpHdr*> THttpHdrList;

class THttpMsg
{
private:
	unsigned int   m_nHttpVer;    // HTTP version (hiword.loword)
	THttpHdrList    m_listHdrs;
public:
	virtual ~THttpMsg( void );
	THttpMsg( void );
	THttpMsg( THttpMsg& other );

	// Total header length for key/val pairs (incl. ": " and CRLF)
	// but NOT separator CRLF
	size_t GetAllHdrLen( void ) ; 
	size_t GetHdrLen( unsigned int nIndex ) ; 
	void ClearAllHdr(void);

	void GetHttpVer( unsigned int* puMajor, unsigned int* puMinor ) const;
	void SetHttpVer( unsigned int uMajor, unsigned int uMinor );

	size_t GetHdrCount( void ) const;
	string   GetHdr( const string& strKey ) ; 
	THttpHdr* GetHdr( unsigned int nIndex ) ; 
	void      SetHdr( const string& strKey, const string& strVal );
	void      SetHdr( const THttpHdr& hdrNew );
	void    ShowAllHttpHdr(void);

	THttpHdrList& GetAllHdr(void) { return m_listHdrs; }

};

}

#endif

