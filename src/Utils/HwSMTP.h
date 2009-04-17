// HwSMTP.h: interface for the CHwSMTP class.
//
//////////////////////////////////////////////////////////////////////

/*******************************************************************
		˵��
	1���������Լ�д��һ������SMTP�����������ʼ����࣬����ʵ�ָ�����
���͡�
	2���� InitInstance() �������� 
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	�� ExitInstance () �е��� ::WSACleanup ();
*******************************************************************/

/****************************************************************************************************************
Ͷ��˵����
****************************************************************************************************************/
#define NOTE_SpeedPostMail \
_T("\
			��������ơ�\r\n\
	����SMTP��������תֱ�ӽ�E-Mail�����ʼ����͵��Է�����\r\n\
			����    ����\r\n\
	1.0.0\r\n\
			������ϵͳ��\r\n\
	Windows ����ϵ��\r\n\
			����    �ߡ�\r\n\
	л��ΰ �� chrys �� chrys@163.com �� http://www.viction.net\r\n\
			�����˵����\r\n\
	���һ����ϤFoxmail�еġ��ؿ�ר�ݡ�������ֱ�ӽ��ʼ����͵��Է����ʼ��������У�������Ҫ����SMTP��������ת��\
�����뽫�����������ؿ�ר�ݡ����͵����ʼ��ķ������������ṩ��������Ϣ��ȡ�࣬���Ի�ȡ����IP��ַ���������롢DNS��\
Wins������MAC��ַ�������Ϣ�����ṩ��SMTPЭ������࣬����ʵ����SMTP�ͻ��˹��ܵ�ʵ���࣬ʵ�ֵ����ʼ��շ���Base64����ʵ��\
���ַ�����ķ�����\r\n\
	����������޸ĸ��Ʊ����룬���뱣��������ֲ�Ҫ�޸ġ�\r\n\
	ϣ������Ϊ�й��������ҵ��һ�ݱ�����\r\n\
			���������ڡ�\r\n\
	2008-11-23 3:54\r\n")

#include <afxsock.h>

#if !defined(AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_)
#define AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CHwSMTP  
{
public:
	CString GetLastErrorText();
	BOOL SendEmail (
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
		CStringArray *pStrAryAttach=NULL,
		CStringArray *pStrAryCC=NULL,
		UINT nSmtpSrvPort=25
		);
	CHwSMTP();
	virtual ~CHwSMTP();

private:
	BOOL SendSubject();
	CStringArray m_StrAryCC;
	BOOL SendHead();
	BOOL auth();
	BOOL SendEmail();
	BOOL Send ( CString &data );
	BOOL SendBuffer(char *buffer,int size=-1);
	BOOL GetResponse( LPCTSTR lpszVerifyCode, int *pnCode=NULL );
	BOOL m_bConnected;
	CSocket m_SendSock;
	CStringArray m_StrAryAttach;
	CString m_csSmtpSrvHost;
	CString m_csUserName;
	CString m_csPasswd;
	CString m_csAddrFrom;
	CString m_csAddrTo;
	CString m_csSenderName;
	CString m_csReceiverName;
	CString m_csSubject;
	CString m_csBody;
private:
	BOOL m_bMustAuth;
	UINT m_nSmtpSrvPort;
	CString m_csCharSet;

	CString m_csLastError;
	BOOL SendOnAttach(LPCTSTR lpszFileName);
	BOOL SendAttach();
	BOOL SendBody();
	CString m_csMIMEContentType;
	CString m_csPartBoundary;
	CString m_csNoMIMEText;

};

BOOL SendEmail (
				  BOOL bViaThreadSend,						// FALSE - ֱ�ӷ��ͣ� TRUE - ���߳��з��ͣ������߳̾��
				  LPCTSTR lpszSmtpSrvHost,					// SMTP ���������磺smtp.21cn.com
				  LPCTSTR lpszUserName,						// ��¼��������֤��ݵ��û���
				  LPCTSTR lpszPasswd,						// ��¼��������֤��ݵ�����
				  BOOL bMustAuth,							// SMTP ��������Ҫ�����֤
				  LPCTSTR lpszAddrFrom,						// ������ E-Mail ��ַ
				  LPCTSTR lpszAddrTo,						// �����ߵ� E-Mail ��ַ
				  LPCTSTR lpszSenderName,					// �����ߵ����֣����磺л��ΰ
				  LPCTSTR lpszReceiverName,					// �����ߵ����֣����磺�ŷ�
				  LPCTSTR lpszSubject,						// �ʼ�����
				  LPCTSTR lpszBody,							// �ʼ�����
				  LPCTSTR lpszCharSet=NULL,					// �ַ������ͣ����磺������������Ӧ����"big5"����������ʱ����"gb2312"
				  CStringArray *pStrAryAttach=NULL,			// �����ļ�·�������Զ��
				  CStringArray *pStrAryCC=NULL,				// ���� E-Mail ��ַ�����Զ��
				  UINT nSmtpSrvPort=25						// SMTP �������� TCP �˿ں�
				  );
void EndOfSMTP ();

#endif // !defined(AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_)
