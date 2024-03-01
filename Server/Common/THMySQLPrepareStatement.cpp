#include "THMySQLPrepareStatement.h"
#include "PxIOCPEngine.h"
#include <fstream>

extern	PxIOCPEngine* g_pIOCPEngine;

THMySQLPrepareStatement::THMySQLPrepareStatement()
{
	m_pCmd = NULL;
}

THMySQLPrepareStatement::~THMySQLPrepareStatement()
{
	Destroy();
}

void	THMySQLPrepareStatement::Destroy()
{
	if (m_pCmd) 
	{ 
		delete m_pCmd;	
		m_pCmd = NULL; 
	}
}

BOOL	THMySQLPrepareStatement::PrepareStatement(sql::Connection* conn, char* strCmd)
{
	try
	{
		m_pCmd = conn->prepareStatement(strCmd);
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "PrepareStatement:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();
	}

	return TRUE;
}

BOOL	THMySQLPrepareStatement::PrepareStatement(sql::Connection* conn, std::string strCmd)
{
	try
	{
		m_pCmd = conn->prepareStatement(strCmd);
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "GetResultSet:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();
	}
	return TRUE;
}

BOOL	THMySQLPrepareStatement::Execute()
{
	try
	{
		BOOL result = m_pCmd->execute();
		return result;
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_excute_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "Execute:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();

		if (e.getErrorCode() == 2013)		// CR_SERVER_LOST
		{
			if (g_pIOCPEngine)
			{
				if (g_pIOCPEngine->m_pDBServerLostCB)
				{
					g_pIOCPEngine->m_pDBServerLostCB(NULL);
				}
			}
		}
	}
	return TRUE;
}

sql::ResultSet* THMySQLPrepareStatement::ExecuteAndGetResultSet()
{
	sql::ResultSet* resultSet = NULL;

	try
	{
		if (m_pCmd->execute())
		{
			resultSet = m_pCmd->getResultSet();
		}
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_excute_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "ExecuteAndGetResultSet:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();

		if (e.getErrorCode() == 2013)		// CR_SERVER_LOST
		{
			if (g_pIOCPEngine)
			{
				if (g_pIOCPEngine->m_pDBServerLostCB)
				{
					g_pIOCPEngine->m_pDBServerLostCB(NULL);
				}
			}
		}
	}

	return resultSet;
}

BOOL	THMySQLPrepareStatement::ExecuteUpdate()
{
	try
	{
		BOOL result = m_pCmd->executeUpdate();
		return result;
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_excute_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "ExecuteUpdate:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();

		if (e.getErrorCode() == 2013)		// CR_SERVER_LOST
		{
			if (g_pIOCPEngine)
			{
				if (g_pIOCPEngine->m_pDBServerLostCB)
				{
					g_pIOCPEngine->m_pDBServerLostCB(NULL);
				}
			}
		}
	}
	return TRUE;
}

void	THMySQLPrepareStatement::SetBigInt(UINT32 parameterIndex, const sql::SQLString& value)
{
	m_pCmd->setBigInt(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetBlob(UINT32 parameterIndex, std::istream* blob)
{
	m_pCmd->setBlob(parameterIndex, blob);
}

void	THMySQLPrepareStatement::SetBoolean(UINT32 parameterIndex, bool value)
{
	m_pCmd->setBoolean(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetDateTime(UINT32 parameterIndex, const sql::SQLString& value)
{
	m_pCmd->setDateTime(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetDouble(UINT32 parameterIndex, double value)
{
	m_pCmd->setDouble(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetInt(UINT32 parameterIndex, int32_t value)
{
	m_pCmd->setInt(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetUInt(UINT32 parameterIndex, uint32_t value)
{
	m_pCmd->setUInt(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetInt64(UINT32 parameterIndex, int64_t value)
{
	m_pCmd->setInt64(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetUInt64(UINT32 parameterIndex, uint64_t value)
{
	m_pCmd->setUInt64(parameterIndex, value);
}

void	THMySQLPrepareStatement::SetNull(UINT32 parameterIndex, int sqlType)
{
	m_pCmd->setNull(parameterIndex, sqlType);
}

void	THMySQLPrepareStatement::SetString(UINT32 parameterIndex, const sql::SQLString& value)
{
	m_pCmd->setString(parameterIndex, value);
}


