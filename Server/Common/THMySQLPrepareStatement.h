#ifndef	_TH_MYSQL_PREPARE_STATEMENT_H_
#define _TH_MYSQL_PREPARE_STATEMENT_H_

#include "PxDefine.h"

#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/prepared_statement.h>

class	THMySQLPrepareStatement
{
public:
	THMySQLPrepareStatement();
	~THMySQLPrepareStatement();

	sql::PreparedStatement*				m_pCmd;

	void				Destroy();

	BOOL				PrepareStatement(sql::Connection* conn, char* strCmd);
	BOOL				PrepareStatement(sql::Connection* conn, std::string strCmd);
	
	BOOL				Execute();
	sql::ResultSet*		ExecuteAndGetResultSet();

	BOOL				ExecuteUpdate();

	void				SetBigInt(UINT32 parameterIndex, const sql::SQLString& value);
	void				SetBlob(UINT32 parameterIndex, std::istream* blob);
	void				SetBoolean(UINT32 parameterIndex, bool value);
	void				SetDateTime(UINT32 parameterIndex, const sql::SQLString& value);
	void				SetDouble(UINT32 parameterIndex, double value);
	void				SetInt(UINT32 parameterIndex, int32_t value);
	void				SetUInt(UINT32 parameterIndex, uint32_t value);
	void				SetInt64(UINT32 parameterIndex, int64_t value);
	void				SetUInt64(UINT32 parameterIndex, uint64_t value);
	void				SetNull(UINT32 parameterIndex, int sqlType);
	void				SetString(UINT32 parameterIndex, const sql::SQLString& value);
};

#endif
