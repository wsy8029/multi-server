#ifndef	_TH_MYSQL_CONNECTOR_H_
#define _TH_MYSQL_CONNECTOR_H_

#include "PxDefine.h"

#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/prepared_statement.h>

class	THMySQLConnector;
typedef BOOL(*PxDBPrepareFunc) (THMySQLConnector* pDBConn);

class	THMySQLConnector
{
	std::string			m_strKey;
	std::string			m_strIV;

public:
	THMySQLConnector();
	~THMySQLConnector();

	sql::Connection*				m_pConn;
	std::string						m_strServerIPTable;
	std::string						m_strHostName;
	UINT16							m_iPort;
	std::string						m_strID;
	std::string						m_strPSWD_Encrypted;
	std::string						m_strSchema;
	std::list<PxDBPrepareFunc>		m_listFuncDBPrepare;

	BOOL				Init(sql::Driver* pDriver, int index, std::string jsonPath, std::string key, std::string iv, PxDBPrepareFunc pFunc);
	void				Destroy();
	void				Update();
	void				FixConnection();

	void				StartDBTransaction();
	void				EndDBTransaction(BOOL bCommit = TRUE);
	void				Rollback(BOOL isSetAutoCommit = FALSE);

	BOOL				ReadOptionFromJson(int index);
};

#endif

