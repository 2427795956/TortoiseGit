#pragma once

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#endif
// _vsnprintf ����
#ifndef _vsnprintf_hw
#ifdef UNICODE
#define _vsnprintf_hw _vsnwprintf
#else
#define _vsnprintf_hw _vsnprintf
#endif
#endif
#define HANDLE_IS_VALID(h) ( (HANDLE)(h)!=NULL && (HANDLE)(h)!=INVALID_HANDLE_VALUE )

template<class T>
int FindFromStaticArray ( IN T *pAry, IN int nArySize, IN T Find )
{
	if ( !pAry ) return -1;
	for ( int i=0; i<nArySize; i++ )
	{
		if ( pAry[i] == Find )
			return i;
	}
	return -1;
}

//
// ע�⣺����Ǵ� CString �в���ʱ Find ǧ��Ҫ�� LPCTSTR ���� char* ������һ����Ҫ�� CString ����
//
template<class T1, class T2>
int FindFromArray ( IN T1 &Ary, IN T2 Find )
{
	int nCount = (int)Ary.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		T2 tGetValue = Ary.GetAt(i);
		if ( tGetValue == Find )
			return i;
	}
	return -1;
}

//
// ������ Ary_Org �в��ң�ֻҪ Ary_Find ���κ�һ��Ԫ���� Ary_Org �г��ֹ�
// �ͷ��ظ�Ԫ���� Ary_Org �е�λ��
//
template<class T1, class T2>
int FindFromArray ( IN T1 &Ary_Org, IN T1 &Ary_Find, OUT T2 &Element )
{
	int nCount = Ary_Find.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		T2 tGetValue = Ary_Find.GetAt(i);
		int nFindPos = FindFromArray ( Ary_Org, tGetValue );
		if ( nFindPos >= 0 )
		{
			Element = Ary_Org.GetAt ( nFindPos );
			return nFindPos;
		}
	}
	return -1;
}

template<class T1, class T2, class T3, class T4>
int FindFromArray ( IN T1 &Ary, IN T2 Find, IN T3 &AppAry, IN T4 AppFind )
{
	int nCount = Ary.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		if ( Ary.GetAt(i) == Find && 
			AppAry.GetAt(i) == AppFind )
		{
			return i;
		}
	}
	return -1;
}

template<class T1>
int FindFromArray ( IN T1 &Ary_Src, IN T1 &Ary_Find )
{
	int nCount = Ary_Src.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		if ( FindFromArray ( Ary_Find, Ary_Src.GetAt(i) ) >= 0 )
			return i;
	}
	return -1;
}

enum
{
	STRING_IS_MULTICHARS = 0,		// �Ƕ��ֽ��ַ���
		STRING_IS_UNICODE,				// ��UNICODE�ַ���
		STRING_IS_SOFTCODE,				// �Ǻͳ���һ�����ַ�������
};

class CMultiByteString
{
public:
	CMultiByteString ( LPCTSTR lpszOrg, int nOrgStringEncodeType=STRING_IS_SOFTCODE, OUT char *pOutBuf=NULL, int nOutBufSize=0 );
	~CMultiByteString ();
	char *GetBuffer()
	{
		if ( m_pszData ) return m_pszData;
		return "";
	}
	// ��ȡ�ַ�������
	int GetLength()
	{
		return m_nCharactersNumber;
	}
	// ��ȡ�ַ���ռ���ڴ��С���ֽ����������ַ���������'\0'��ռ��λ�ã�
	int GetSize()
	{
		return m_nDataSize;
	}
private:
	char *m_pszData;			// ����Ŀ���ַ����Ļ���
	int m_nDataSize;			// Ŀ���ַ���ռ�õ��ڴ��С���ֽ����������ַ���������'\0'��
	int m_nCharactersNumber;	// Ŀ���ַ����ĸ���
	BOOL m_bNewBuffer;			// �Ƿ�����������������ڴ�
};

CString GetCompatibleString ( LPVOID lpszOrg, BOOL bOrgIsUnicode, int nOrgLength=-1 );
CString FormatDateTime ( COleDateTime &DateTime, LPCTSTR pFormat );
CString FormatString ( LPCTSTR lpszStr, ... );
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus=NULL );
CString FormatBytes ( double fBytesNum, BOOL bShowUnit=TRUE, int nFlag=0 );
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime=10*1000 );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.