// HwSMTP.cpp: implementation of the CHwSMTP class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "afxstr.h"
#include "HwSMTP.h"
#include "CBase64.h"
#include "SpeedPostEmail.h"

#include <Afxmt.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPtrArray g_PtrAry_Threads;
::CCriticalSection m_CSFor__g_PtrAry_Threads;

class CEMailObject
{
public:
	CEMailObject (
		LPCTSTR lpszSmtpSrvHost,
		LPCTSTR lpszUserName,
		LPCTSTR lpszPasswd,
		BOOL bMustAuth,
		LPCTSTR lpszAddrFrom,
		LPCTSTR lpszAddrTo,
		LPCTSTR lpszSenderName,
		LPCTSTR lpszReceiverName,
		LPCTSTR lpszSubject,
		LPCTSTR lpszBody,
		LPCTSTR lpszCharSet,
		CStringArray *pStrAryAttach,
		CStringArray *pStrAryCC,
		UINT nSmtpSrvPort
		)
	{
		m_csSmtpSrvHost = GET_SAFE_STRING(lpszSmtpSrvHost);
		m_csUserName = GET_SAFE_STRING(lpszUserName);
		m_csPasswd = GET_SAFE_STRING(lpszPasswd);
		m_bMustAuth = bMustAuth;
		m_csAddrFrom = GET_SAFE_STRING(lpszAddrFrom);
		m_csAddrTo = GET_SAFE_STRING(lpszAddrTo);
		m_csSenderName = GET_SAFE_STRING(lpszSenderName);
		m_csReceiverName = GET_SAFE_STRING(lpszReceiverName);
		m_csSubject = GET_SAFE_STRING(lpszSubject);
		m_csBody = GET_SAFE_STRING(lpszBody);
		m_csCharSet = GET_SAFE_STRING(lpszCharSet);
		if ( pStrAryAttach )
			m_StrAryAttach.Append ( *pStrAryAttach );
		if ( pStrAryCC )
			m_StrAryCC.Append ( *pStrAryCC );
		m_nSmtpSrvPort = nSmtpSrvPort;
		m_hThread = NULL;
	}

public:
	CString m_csSmtpSrvHost;
	CString m_csUserName;
	CString m_csPasswd;
	BOOL m_bMustAuth;
	CString m_csAddrFrom;
	CString m_csAddrTo;
	CString m_csSenderName;
	CString m_csReceiverName;
	CString m_csSubject;
	CString m_csBody;
	CString m_csCharSet;
	CStringArray m_StrAryAttach;
	CStringArray m_StrAryCC;
	UINT m_nSmtpSrvPort;

	HANDLE m_hThread;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHwSMTP::CHwSMTP () :
	m_bConnected ( FALSE ),
	m_nSmtpSrvPort ( 25 ),
	m_bMustAuth ( TRUE )
{
	m_csMIMEContentType = _T( "multipart/mixed");
	m_csPartBoundary = _T( "WC_MAIL_PaRt_BoUnDaRy_05151998" );
	m_csNoMIMEText = _T( "This is a multi-part message in MIME format." );
//	m_csCharSet = _T("\r\n\tcharset=\"iso-8859-1\"\r\n");
}

CHwSMTP::~CHwSMTP()
{
}

BOOL CHwSMTP::SendEmail (
		LPCTSTR lpszSmtpSrvHost,
		LPCTSTR lpszUserName,
		LPCTSTR lpszPasswd,
		BOOL bMustAuth,
		LPCTSTR lpszAddrFrom,
		LPCTSTR lpszAddrTo,
		LPCTSTR lpszSenderName,
		LPCTSTR lpszReceiverName,
		LPCTSTR lpszSubject,
		LPCTSTR lpszBody,
		LPCTSTR lpszCharSet,						// �ַ������ͣ����磺������������Ӧ����"big5"����������ʱ����"gb2312"
		CStringArray *pStrAryAttach/*=NULL*/,
		CStringArray *pStrAryCC/*=NULL*/,
		UINT nSmtpSrvPort/*=25*/
		)
{
	TRACE ( _T("�����ʼ���%s, %s, %s\n"), lpszAddrTo, lpszReceiverName, lpszBody );
	m_StrAryAttach.RemoveAll();
	m_StrAryCC.RemoveAll();
	m_csSmtpSrvHost = GET_SAFE_STRING ( lpszSmtpSrvHost );
	if ( m_csSmtpSrvHost.GetLength() <= 0 )
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}
	m_csUserName = GET_SAFE_STRING ( lpszUserName );
	m_csPasswd = GET_SAFE_STRING ( lpszPasswd );
	m_bMustAuth = bMustAuth;
	if ( m_bMustAuth && m_csUserName.GetLength() <= 0 )
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}

	m_csAddrFrom = GET_SAFE_STRING ( lpszAddrFrom );
	m_csAddrTo = GET_SAFE_STRING ( lpszAddrTo );
	m_csSenderName = GET_SAFE_STRING ( lpszSenderName );
	m_csReceiverName = GET_SAFE_STRING ( lpszReceiverName );
	m_csSubject = GET_SAFE_STRING ( lpszSubject );
	m_csBody = GET_SAFE_STRING ( lpszBody );
	m_nSmtpSrvPort = nSmtpSrvPort;
	if ( lpszCharSet && lstrlen(lpszCharSet) > 0 )
		m_csCharSet.Format ( _T("\r\n\tcharset=\"%s\"\r\n"), lpszCharSet );

	if	(
			m_csAddrFrom.GetLength() <= 0 || m_csAddrTo.GetLength() <= 0 ||
			m_csSenderName.GetLength() <= 0 || m_csReceiverName.GetLength() <= 0 ||
			m_csSubject.GetLength() <= 0 || m_csBody.GetLength() <= 0
		)
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}

	if ( pStrAryAttach )
	{
		m_StrAryAttach.Append ( *pStrAryAttach );
	}
	if ( m_StrAryAttach.GetSize() < 1 )
		m_csMIMEContentType = _T( "text/plain");

	if ( pStrAryCC )
	{
		m_StrAryCC.Append ( *pStrAryCC );
	}	

	// ����Socket
	if ( !m_SendSock.Create () )
	{
		m_csLastError.Format ( _T("Create socket failed!") );
		return FALSE;
	}

	// ���ӵ�������
	if ( !m_SendSock.Connect ( m_csSmtpSrvHost, m_nSmtpSrvPort ) )
	{
		m_csLastError.Format ( _T("Connect to [ %s ] failed"), m_csSmtpSrvHost );
		TRACE ( _T("%d\n"), GetLastError() );
		return FALSE;
	}
	if ( !GetResponse( _T("220") ) ) return FALSE;

	m_bConnected = TRUE;
	return SendEmail();

}


BOOL CHwSMTP::GetResponse ( LPCTSTR lpszVerifyCode, int *pnCode/*=NULL*/)
{
	if ( !lpszVerifyCode || lstrlen(lpszVerifyCode) < 1 )
		return FALSE;

	char szRecvBuf[1024] = {0};
	int nRet = 0;
	char szStatusCode[4] = {0};
	nRet = m_SendSock.Receive ( szRecvBuf, sizeof(szRecvBuf) );
	TRACE ( _T("Received : %s\r\n"), szRecvBuf );
	if ( nRet <= 0 )
	{
		m_csLastError.Format ( _T("Receive TCP data failed") );
		return FALSE;
	}
//	TRACE ( _T("�յ���������Ӧ��%s\n"), szRecvBuf );

	memcpy ( szStatusCode, szRecvBuf, 3 );
	if ( pnCode ) (*pnCode) = atoi ( szStatusCode );

	if ( strcmp ( szStatusCode, CMultiByteString(lpszVerifyCode).GetBuffer() ) != 0 )
	{
		m_csLastError.Format ( _T("Received invalid response  : %s"), GetCompatibleString(szRecvBuf,FALSE) );
		return FALSE;
	}

	return TRUE;
}
BOOL CHwSMTP::SendBuffer(char *buff,int size)
{
	if(size<0)
		size=strlen(buff);
	if ( !m_bConnected )
	{
		m_csLastError.Format ( _T("Didn't connect") );
		return FALSE;
	}

	if ( m_SendSock.Send ( buff, size ) != size )
	{
		m_csLastError.Format ( _T("Socket send data failed") );
		return FALSE;
	}
	
	return TRUE;
}
// ����socket�������ݣ����ݳ��Ȳ��ܳ���10M
BOOL CHwSMTP::Send(LPCTSTR lpszData, ... )
{
	if ( !m_bConnected )
	{
		m_csLastError.Format ( _T("Didn't connect") );
		return FALSE;
	}

	TCHAR *buf = NULL;
	// ѭ����ʽ���ַ�����������岻���򽫻���Ӵ�Ȼ�������Ա�֤���幻��ͬʱ�ֲ��˷�
	for ( int nBufCount = 1024; nBufCount<10*1024*1024; nBufCount += 1024 )
	{
		buf = new TCHAR[nBufCount];
		if ( !buf )
		{
			::AfxThrowMemoryException ();
			return FALSE;
		}
		memset ( buf, 0, nBufCount*sizeof(TCHAR) );

		va_list  va;
		va_start (va, lpszData);
		int nLen = _vsnprintf_hw ((TCHAR*)buf, nBufCount-sizeof(TCHAR), lpszData, va);
		va_end(va);
		if ( nLen <= (int)(nBufCount-sizeof(TCHAR)) )
			break;
		delete[] buf; buf = NULL;
	}
	if ( !buf )
	{
		m_csLastError.Format ( _T("Memory too small or data too big.") );
		return FALSE;
	}

	CMultiByteString cbsData ( buf );
	delete[] buf; buf = NULL;
	TRACE ( _T("Send : %s\r\n"), cbsData.GetBuffer() );
	if ( m_SendSock.Send ( cbsData.GetBuffer(), cbsData.GetLength() ) != cbsData.GetLength() )
	{
		m_csLastError.Format ( _T("Socket send data failed") );
		return FALSE;
	}
	
	return TRUE;
}

BOOL CHwSMTP::SendEmail()
{
	BOOL bRet = TRUE;
	char szLocalHostName[64] = {0};
	gethostname ( (char*)szLocalHostName, sizeof(szLocalHostName) );

	// hello������
	if ( !Send ( _T("HELO %s\r\n"), GetCompatibleString(szLocalHostName,FALSE) ) )
	{
		return FALSE;
	}
	if ( !GetResponse ( _T("250") ) )
	{
		return FALSE;
	}
	// �����֤
	if ( m_bMustAuth && !auth() )
	{
		return FALSE;
	}
	// �����ʼ�ͷ
	if ( !SendHead() )
	{
		return FALSE;
	}
	// �����ʼ�����
	if ( !SendSubject() )
	{
		return FALSE;
	}
	// �����ʼ�����
	if ( !SendBody() )
	{
		return FALSE;
	}
	// ���͸���
	if ( !SendAttach() )
	{
		return FALSE;
	}
	// �����ʼ�����
	if ( !Send ( _T(".\r\n") ) ) return FALSE;
	if ( !GetResponse ( _T("250") ) )
		return FALSE;

	// �˳�����
	if ( HANDLE_IS_VALID(m_SendSock.m_hSocket) )
		Send ( _T("QUIT\r\n") );
	m_bConnected = FALSE;

	return bRet;
}

BOOL CHwSMTP::auth()
{
	int nResponseCode = 0;
	if ( !Send ( _T("auth login\r\n") ) ) return FALSE;
	if ( !GetResponse ( _T("334"), &nResponseCode ) ) return FALSE;
	if ( nResponseCode != 334 )	// ����Ҫ��֤�û���������
		return TRUE;
	
	CBase64 Base64Encode;
	CMultiByteString cbsUserName ( m_csUserName ), cbsPasswd ( m_csPasswd );
	CString csBase64_UserName = GetCompatibleString ( Base64Encode.Encode ( cbsUserName.GetBuffer(), cbsUserName.GetLength() ).GetBuffer(0), FALSE );
	CString csBase64_Passwd = GetCompatibleString ( Base64Encode.Encode ( cbsPasswd.GetBuffer(), cbsPasswd.GetLength() ).GetBuffer(0), FALSE );

	if ( !Send ( _T("%s\r\n"), csBase64_UserName ) ) return FALSE;
	if ( !GetResponse ( _T("334") ) )
	{
		m_csLastError.Format ( _T("Authentication UserName failed") );
		return FALSE;
	}

	if ( !Send ( _T("%s\r\n"), csBase64_Passwd ) ) return FALSE;
	if ( !GetResponse ( _T("235") ) )
	{
		m_csLastError.Format ( _T("Authentication Password failed") );
		return FALSE;
	}

	return TRUE;
}

BOOL CHwSMTP::SendHead()
{
	if ( !Send ( _T("MAIL From: <%s>\r\n"), m_csAddrFrom ) ) return FALSE;
	if ( !GetResponse ( _T("250") ) ) return FALSE;
	if ( !Send ( _T("RCPT TO: <%s>\r\n"), m_csAddrTo ) ) return FALSE;
	if ( !GetResponse ( _T("250") ) ) return FALSE;

	for ( int i=0; i<m_StrAryCC.GetSize(); i++ )
	{
		if ( !Send ( _T("RCPT TO: <%s>\r\n"), m_StrAryCC.GetAt(i) ) ) return FALSE;
		if ( !GetResponse ( _T("250") ) ) return FALSE;
	}
	if ( !Send ( _T("DATA\r\n") ) ) return FALSE;
	if ( !GetResponse ( _T("354") ) ) return FALSE;	

	return TRUE;
}

BOOL CHwSMTP::SendSubject()
{
	CString csSubject;
	csSubject += _T("Date: ");
	COleDateTime tNow = COleDateTime::GetCurrentTime();
	if ( tNow > 1 )
	{
		csSubject += GetCompatibleString(FormatDateTime (tNow, _T("%a, %d %b %y %H:%M:%S %Z")).GetBuffer(0),FALSE);
	}
	csSubject += _T("\r\n");
	csSubject += FormatString ( _T("From: %s\r\nTo: %s\r\n"), m_csSenderName, m_csReceiverName );
	csSubject += FormatString ( _T("Subject: %s\r\n"), m_csSubject );
	csSubject += FormatString ( _T("X-Mailer: chrys\r\nMIME-Version: 1.0\r\nContent-Type: %s; %s boundary=%s\r\n\r\n") , 
		m_csMIMEContentType, m_csCharSet, m_csPartBoundary );

	return Send ( csSubject );
}

BOOL CHwSMTP::SendBody()
{
	CString csBody, csTemp;

	if ( m_StrAryAttach.GetSize() > 0 )
	{
		csTemp.Format ( _T("%s\r\n\r\n"), m_csNoMIMEText );
		csBody += csTemp;
		
		csTemp.Format ( _T("--%s\r\n"), m_csPartBoundary );
		csBody += csTemp;
		
		csTemp.Format ( _T("Content-Type: text/plain\r\n%sContent-Transfer-Encoding: 7Bit\r\n\r\n"),
			m_csCharSet );
		csBody += csTemp;
	}

	csTemp.Format ( _T("%s\r\n"), m_csBody );
	csBody += csTemp;

	if ( m_StrAryAttach.GetSize() > 0 )
	{
		csTemp.Format ( _T("--%s\r\n"), m_csPartBoundary );
		csBody += csTemp;
	}

	return Send ( csBody );
}

BOOL CHwSMTP::SendAttach()
{
	int nCountAttach = (int)m_StrAryAttach.GetSize();
	if ( nCountAttach < 1 ) return TRUE;

	for ( int i=0; i<nCountAttach; i++ )
	{
		if ( !SendOnAttach ( m_StrAryAttach.GetAt(i) ) )
			return FALSE;
	}

	return TRUE;
}

BOOL CHwSMTP::SendOnAttach(LPCTSTR lpszFileName)
{
	ASSERT ( lpszFileName );
	CString csAttach, csTemp;

	csTemp = lpszFileName;
	CString csShortFileName = csTemp.GetBuffer(0) + csTemp.ReverseFind ( '\\' );
	csShortFileName.TrimLeft ( _T("\\") );

	csTemp.Format ( _T("Content-Type: application/octet-stream; file=%s\r\n"), csShortFileName );
	csAttach += csTemp;

	csTemp.Format ( _T("Content-Transfer-Encoding: base64\r\n") );
	csAttach += csTemp;

	csTemp.Format ( _T("Content-Disposition: attachment; filename=%s\r\n\r\n"), csShortFileName );
	csAttach += csTemp;

	DWORD dwFileSize =  hwGetFileAttr(lpszFileName);
	if ( dwFileSize > 5*1024*1024 )
	{
		m_csLastError.Format ( _T("File [%s] too big. File size is : %s"), lpszFileName, FormatBytes(dwFileSize) );
		return FALSE;
	}
	char *pBuf = new char[dwFileSize+1];
	if ( !pBuf ) 
	{
		::AfxThrowMemoryException ();
		return FALSE;
	}

	if(!Send ( csAttach ))
		return FALSE;

	CFile file;
	CStringA filedata;
	try
	{
		if ( !file.Open ( lpszFileName, CFile::modeRead ) )
		{
			m_csLastError.Format ( _T("Open file [%s] failed"), lpszFileName );
			return FALSE;
		}
		UINT nFileLen = file.Read ( pBuf, dwFileSize );
		CBase64 Base64Encode;
		filedata = Base64Encode.Encode ( pBuf, nFileLen );
		filedata += _T("\r\n\r\n");
	}
	catch ( CFileException e )
	{
		e.Delete();
		m_csLastError.Format ( _T("Read file [%s] failed"), lpszFileName );
		delete[] pBuf;
		return FALSE;
	}

	if(!SendBuffer( filedata.GetBuffer() ))
		return FALSE;

	csAttach.Empty();
	csTemp.Format ( _T("--%s\r\n"), m_csPartBoundary );
	csAttach += csTemp;

	delete[] pBuf;
	
	return Send ( csAttach );
}

CString CHwSMTP::GetLastErrorText()
{
	return m_csLastError;
}


DWORD WINAPI ThreadProc_SendEmail( LPVOID lpParameter )
{
	CEMailObject *pEMailObject = (CEMailObject*)lpParameter;
	ASSERT ( pEMailObject );

	CHwSMTP HwSMTP;
	BOOL bRet = HwSMTP.SendEmail (
		pEMailObject->m_csSmtpSrvHost,
		pEMailObject->m_csUserName,
		pEMailObject->m_csPasswd,
		pEMailObject->m_bMustAuth,
		pEMailObject->m_csAddrFrom,
		pEMailObject->m_csAddrTo,
		pEMailObject->m_csSenderName,
		pEMailObject->m_csReceiverName,
		pEMailObject->m_csSubject,
		pEMailObject->m_csBody,
		pEMailObject->m_csCharSet,
		&pEMailObject->m_StrAryAttach,
		&pEMailObject->m_StrAryCC,
		pEMailObject->m_nSmtpSrvPort
		);
	if ( !bRet)
	{
#ifdef _DEBUG
		CString csError = HwSMTP.GetLastErrorText ();
		csError = FormatString ( _T("Send a email to [%s] failed."), pEMailObject->m_csSmtpSrvHost );
		AfxMessageBox ( csError );
#endif
	}

	m_CSFor__g_PtrAry_Threads.Lock ();
	int nFindPos = FindFromArray ( g_PtrAry_Threads, pEMailObject->m_hThread );
	if ( nFindPos >= 0 )
		g_PtrAry_Threads.RemoveAt ( nFindPos );
	m_CSFor__g_PtrAry_Threads.Unlock ();

	delete pEMailObject;
	return bRet;
}

//
// �� SMTP �����͵����ʼ���������ò��� bViaThreadSend=TRUE�����ڳ������ʱӦ���� ExitInstance() �е��� EndOfSMTP() ����
//
BOOL SendEmail (
				BOOL bViaThreadSend,
				LPCTSTR lpszSmtpSrvHost,
				LPCTSTR lpszUserName,
				LPCTSTR lpszPasswd,
				BOOL bMustAuth,
				LPCTSTR lpszAddrFrom,
				LPCTSTR lpszAddrTo,
				LPCTSTR lpszSenderName,
				LPCTSTR lpszReceiverName,
				LPCTSTR lpszSubject,
				LPCTSTR lpszBody,
				LPCTSTR lpszCharSet/*=NULL*/,
				CStringArray *pStrAryAttach/*=NULL*/,
				CStringArray *pStrAryCC/*=NULL*/,
				UINT nSmtpSrvPort/*=25*/
				)
{
	if ( !lpszSmtpSrvHost || lstrlen(lpszSmtpSrvHost) < 1 ||
		!lpszSubject || lstrlen(lpszSubject) < 1 ||
		!lpszBody || lstrlen(lpszBody) < 1 )
	{
		AfxMessageBox ( _T("Parameter error !") );
		return FALSE;
	}

	CEMailObject *pEMailObject = new CEMailObject (
		lpszSmtpSrvHost,
		lpszUserName,
		lpszPasswd,
		bMustAuth,
		lpszAddrFrom,
		lpszAddrTo,
		lpszSenderName,
		lpszReceiverName,
		lpszSubject,
		lpszBody,
		lpszCharSet,
		pStrAryAttach,
		pStrAryCC,
		nSmtpSrvPort
		);
	if ( !pEMailObject ) return FALSE;

	BOOL bRet = FALSE;
	if ( bViaThreadSend )
	{
		DWORD dwThreadId = 0;
		pEMailObject->m_hThread = ::CreateThread ( NULL, 0, ::ThreadProc_SendEmail, pEMailObject, CREATE_SUSPENDED, &dwThreadId );
		bRet = HANDLE_IS_VALID(pEMailObject->m_hThread);
		m_CSFor__g_PtrAry_Threads.Lock();
		g_PtrAry_Threads.Add ( pEMailObject->m_hThread );
		m_CSFor__g_PtrAry_Threads.Unlock();
		ResumeThread ( pEMailObject->m_hThread );
	}
	else
	{
		bRet = (BOOL)ThreadProc_SendEmail ( pEMailObject );
	}

	return bRet;
}

void EndOfSMTP ()
{
	// �ȴ������߳�ִ�����
	for ( int i=0; i<g_PtrAry_Threads.GetSize(); i++ )
	{
		HANDLE hThread = (HANDLE)g_PtrAry_Threads.GetAt(i);
		if ( HANDLE_IS_VALID(hThread) )
		{
			WaitForThreadEnd ( &hThread, 30*1000 );
		}
	}
	g_PtrAry_Threads.RemoveAll ();
}


//
// ���ַ��� lpszOrg ת��Ϊ���ֽڵ��ַ����������Ҫʹ�ö��ַ����ĳ��ȣ����������·�ʽ��ʹ������ࣺ
// CMultiByteString MultiByteString(_T("UNICODE�ַ���"));
// printf ( "ANSI �ַ���Ϊ�� %s�� �ַ�����Ϊ�� %d �� ����Ϊ�� %d�ֽ�\n", MultiByteString.GetBuffer(), MultiByteString.GetLength(), MultiByteString.GetSize() );
//
CMultiByteString::CMultiByteString( LPCTSTR lpszOrg, int nOrgStringEncodeType/*=STRING_IS_SOFTCODE*/, OUT char *pOutBuf/*=NULL*/, int nOutBufSize/*=0*/ )
{
	m_bNewBuffer = FALSE;
	m_pszData = NULL;
	m_nDataSize = 0;
	m_nCharactersNumber = 0;
	if ( !lpszOrg ) return;
	
	// �ж�ԭʼ�ַ����ı��뷽ʽ
	BOOL bOrgIsUnicode = FALSE;
	if ( nOrgStringEncodeType == STRING_IS_MULTICHARS ) bOrgIsUnicode = FALSE;
	else if ( nOrgStringEncodeType == STRING_IS_UNICODE ) bOrgIsUnicode = TRUE;
	else
	{
#ifdef UNICODE
		bOrgIsUnicode = TRUE;
#else
		bOrgIsUnicode = FALSE;
#endif
	}
	
	// �����ַ�����������Ҫ��Ŀ�껺���С
	if ( bOrgIsUnicode )
	{
		m_nCharactersNumber = (int)wcslen((WCHAR*)lpszOrg);
		m_nDataSize = (m_nCharactersNumber + 1) * sizeof(WCHAR);
	}
	else
	{
		m_nCharactersNumber = (int)strlen((char*)lpszOrg);
		m_nDataSize = (m_nCharactersNumber + 1) * sizeof(char);
	}
	
	// ʹ�õ����ߴ���Ļ���
	if ( pOutBuf && nOutBufSize > 0 )
	{
		m_pszData = pOutBuf;
		m_nDataSize = nOutBufSize;
	}
	// �Լ������ڴ滺��
	else
	{
		m_pszData = (char*)new BYTE[m_nDataSize];
		if ( !m_pszData )
		{
			::AfxThrowMemoryException ();
			return;
		}
		m_bNewBuffer = TRUE;
	}
	memset ( m_pszData, 0, m_nDataSize );
	
	if ( bOrgIsUnicode )
	{
		m_nCharactersNumber = WideCharToMultiByte ( CP_ACP, 0, (LPCWSTR)lpszOrg, m_nCharactersNumber, (LPSTR)m_pszData, m_nDataSize/sizeof(char)-1, NULL, NULL );
		if ( m_nCharactersNumber < 1 ) m_nCharactersNumber = (int)strlen ( m_pszData );
	}
	else
	{
		m_nCharactersNumber = __min ( m_nCharactersNumber, (int)(m_nDataSize/sizeof(char)-1) );
		strncpy ( m_pszData, (const char*)lpszOrg, m_nCharactersNumber );
		m_nCharactersNumber = (int)strlen ( m_pszData );
	}
	m_nDataSize = ( m_nCharactersNumber + 1 ) * sizeof(char);
}

CMultiByteString::~CMultiByteString ()
{
	if ( m_bNewBuffer && m_pszData )
	{
		delete[] m_pszData;
	}
}

//
// �� lpszOrg ת��Ϊ�ó���ʹ�õı����ַ���������ó����� UNICODE ��תΪ UNICODE������� ANSI ��תΪ ANSI ��
//
CString GetCompatibleString ( LPVOID lpszOrg, BOOL bOrgIsUnicode, int nOrgLength/*=-1*/ )
{
	if ( !lpszOrg ) return _T("");
	
	TRY
	{
#ifdef UNICODE
		if ( bOrgIsUnicode )
		{
			if ( nOrgLength > 0 )
			{
				WCHAR *szRet = new WCHAR[nOrgLength+1];
				if ( !szRet ) return _T("");
				memset ( szRet, 0, (nOrgLength+1)*sizeof(WCHAR) );
				memcpy ( szRet, lpszOrg, nOrgLength*sizeof(WCHAR) );
				CString csRet = szRet;
				delete[] szRet;
				return csRet;
			}
			else if ( nOrgLength == 0 )
				return _T("");
			else
				return (LPCTSTR)lpszOrg;
		}
		
		if ( nOrgLength < 0 )
			nOrgLength = (int)strlen((const char*)lpszOrg);
		int nWideCount = nOrgLength + 1;
		WCHAR *wchar = new WCHAR[nWideCount];
		if ( !wchar ) return _T("");
		memset ( wchar, 0, nWideCount*sizeof(WCHAR) );
		::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpszOrg, nOrgLength, wchar, nWideCount);
		CString csRet = wchar;
		delete[] wchar;
		return csRet;
#else
		if ( !bOrgIsUnicode )
		{
			if ( nOrgLength > 0 )
			{
				char *szRet = new char[nOrgLength+1];
				if ( !szRet ) return _T("");
				memset ( szRet, 0, (nOrgLength+1)*sizeof(char) );
				memcpy ( szRet, lpszOrg, nOrgLength*sizeof(char) );
				CString csRet = szRet;
				delete[] szRet;
				return csRet;
			}
			else if ( nOrgLength == 0 )
				return _T("");
			else
				return (LPCTSTR)lpszOrg;
		}
		
		if ( nOrgLength < 0 )
			nOrgLength = (int)wcslen((WCHAR*)lpszOrg);
		int nMultiByteCount = nOrgLength + 1;
		char *szMultiByte = new char[nMultiByteCount];
		if ( !szMultiByte ) return _T("");
		memset ( szMultiByte, 0, nMultiByteCount*sizeof(char) );
		::WideCharToMultiByte ( CP_ACP, 0, (LPCWSTR)lpszOrg, nOrgLength, (LPSTR)szMultiByte, nMultiByteCount, NULL, NULL );
		CString csRet = szMultiByte;
		delete[] szMultiByte;
		return csRet;
#endif
	}
	CATCH_ALL(e)
	{
		THROW_LAST ();
	}
	END_CATCH_ALL
		
		return _T("");
}

CString FormatDateTime ( COleDateTime &DateTime, LPCTSTR pFormat )
{	
	// If null, return empty string
	if ( DateTime.GetStatus() == COleDateTime::null || DateTime.GetStatus() == COleDateTime::invalid )
		return _T("");
	
	UDATE ud;
	if (S_OK != VarUdateFromDate(DateTime.m_dt, 0, &ud))
	{
		return _T("");
	}
	
	struct tm tmTemp;
	tmTemp.tm_sec	= ud.st.wSecond;
	tmTemp.tm_min	= ud.st.wMinute;
	tmTemp.tm_hour	= ud.st.wHour;
	tmTemp.tm_mday	= ud.st.wDay;
	tmTemp.tm_mon	= ud.st.wMonth - 1;
	tmTemp.tm_year	= ud.st.wYear - 1900;
	tmTemp.tm_wday	= ud.st.wDayOfWeek;
	tmTemp.tm_yday	= ud.wDayOfYear - 1;
	tmTemp.tm_isdst	= 0;
	
	CString strDate;
	LPTSTR lpszTemp = strDate.GetBufferSetLength(256);
	_tcsftime(lpszTemp, strDate.GetLength(), pFormat, &tmTemp);
	strDate.ReleaseBuffer();
	
	return strDate;
}

CString FormatString ( LPCTSTR lpszStr, ... )
{
	TCHAR *buf = NULL;
	// ѭ����ʽ���ַ�����������岻���򽫻���Ӵ�Ȼ�������Ա�֤���幻��ͬʱ�ֲ��˷�
	for ( int nBufCount = 1024; nBufCount<5*1024*1024; nBufCount += 1024 )
	{
		buf = new TCHAR[nBufCount];
		if ( !buf )
		{
			::AfxThrowMemoryException ();
			return _T("");
		}
		memset ( buf, 0, nBufCount*sizeof(TCHAR) );
		
		va_list  va;
		va_start (va, lpszStr);
		int nLen = _vsnprintf_hw ((TCHAR*)buf, nBufCount-sizeof(TCHAR), lpszStr, va);
		va_end(va);
		if ( nLen <= (int)(nBufCount-sizeof(TCHAR)) )
			break;
		delete[] buf; buf = NULL;
	}
	if ( !buf )
	{
		return _T("");
	}
	
	CString csMsg = buf;
	delete[] buf; buf = NULL;
	return csMsg;
}

/********************************************************************************
* Function Type	:	Global
* Parameter		:	lpFileName	-	�ļ�·��
* Return Value	:	-1			-	ʧ��
*					>=0			-	�ļ���С
* Description	:	��ȡ�ļ����� ( �ļ���С������ʱ�� )
*********************************************************************************/
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus/*=NULL*/ )
{
	if ( !lpFileName || lstrlen(lpFileName) < 1 ) return -1;
	
	CFileStatus fileStatus;
	fileStatus.m_attribute = 0;
	fileStatus.m_size = 0;
	memset ( fileStatus.m_szFullName, 0, sizeof(fileStatus.m_szFullName) );
	BOOL bRet = FALSE;
	TRY
	{
		if ( CFile::GetStatus(lpFileName,fileStatus) )
		{
			bRet = TRUE;
		}
	}
	CATCH (CFileException, e)
	{
		ASSERT ( FALSE );
		bRet = FALSE;
	}
	CATCH_ALL(e)
	{
		ASSERT ( FALSE );
		bRet = FALSE;
	}
	END_CATCH_ALL;
	
	if ( pFileStatus )
	{
		pFileStatus->m_ctime = fileStatus.m_ctime;
		pFileStatus->m_mtime = fileStatus.m_mtime;
		pFileStatus->m_atime = fileStatus.m_atime;
		pFileStatus->m_size = fileStatus.m_size;
		pFileStatus->m_attribute = fileStatus.m_attribute;
		pFileStatus->_m_padding = fileStatus._m_padding;
		lstrcpy ( pFileStatus->m_szFullName, fileStatus.m_szFullName );
		
	}
	
	return (int)fileStatus.m_size;
}

//
// ��һ����ʾ�ֽڵ����ÿɶ��Ժõ��ַ�������ʾ�����罫 12345678 �ֽ�ת��Ϊ��
// 11.77M
// nFlag	-	0 : �Զ�ƥ�䵥λ
//				1 : �� Kb Ϊ��λ
//				2 : �� Mb Ϊ��λ
//				3 : �� Gb Ϊ��λ
// 
CString FormatBytes ( double fBytesNum, BOOL bShowUnit/*=TRUE*/, int nFlag/*=0*/ )
{
	CString csRes;
	if ( nFlag == 0 )
	{
		if ( fBytesNum >= 1024.0 && fBytesNum < 1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / 1024.0, bShowUnit?_T(" K"):_T("") );
		else if ( fBytesNum >= 1024.0*1024.0 && fBytesNum < 1024.0*1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0), bShowUnit?_T(" M"):_T("") );
		else if ( fBytesNum >= 1024.0*1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0*1024.0), bShowUnit?_T(" G"):_T("") );
		else
			csRes.Format ( _T("%.2f%s"), fBytesNum, bShowUnit?_T(" B"):_T("") );
	}
	else if ( nFlag == 1 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / 1024.0, bShowUnit?_T(" K"):_T("") );
	}
	else if ( nFlag == 2 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0), bShowUnit?_T(" M"):_T("") );
	}
	else if ( nFlag == 3 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0*1024.0), bShowUnit?_T(" G"):_T("") );
	}
	
	return csRes;
}

//
// �ȴ��߳��˳�
//
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime /*=10*1000*/ )
{
	BOOL bRet = TRUE;
	ASSERT ( phThread );
	if ( !(*phThread) ) return TRUE;
	if ( ::WaitForSingleObject ( *phThread, dwWaitTime ) == WAIT_TIMEOUT )
	{
		bRet = FALSE;
		::TerminateThread ( *phThread, 0 );
	}
	::CloseHandle ( *phThread );
	(*phThread) = NULL;
	return bRet;
}
