#include "THMySQLConnector.h"
#include "PxIOCPEngine.h"

#include "rapidjson/document.h"
#include "rapidjson/reader.h"

#include "../ServerCommon/MyCommonDefine.h"
#include <fstream>

extern	PxIOCPEngine* g_pIOCPEngine;

THMySQLConnector::THMySQLConnector()
{
	m_pConn = NULL;
}

THMySQLConnector::~THMySQLConnector()
{
	Destroy();
}

BOOL	THMySQLConnector::Init(sql::Driver* pDriver, int index, std::string jsonPath, std::string key, std::string iv, PxDBPrepareFunc pFunc)
{
	if (ReadOptionFromJson(index) == FALSE)
	{
		return FALSE;
	}

	m_strKey = key;
	m_strIV = iv;

	std::string pswd_decrypted = AES_Decrypt(m_strPSWD_Encrypted.c_str(), m_strKey, m_strIV);

	/*sql::ConnectOptionsMap connection_properties;
	connection_properties["hostName"] = m_strHostName.c_str();
	connection_properties["userName"] = m_strID.c_str();
	connection_properties["password"] = pswd_decrypted.c_str();
	connection_properties["schema"] = m_strSchema;
	connection_properties["port"] = m_iPort;
	connection_properties["OPT_RECONNECT"] = true;
	connection_properties["characterSetResults"] = "utf8";
	connection_properties["OPT_CHARSET_NAME"] = "utf8";
	connection_properties["OPT_SET_CHARSET_NAME"] = "utf8";

	m_pConn = pDriver->connect(connection_properties);
	if (m_pConn == NULL)
	{
		g_pIOCPEngine->dbgprint("mysql connect failed\n");
		return FALSE;
	}*/

	m_pConn = pDriver->connect(m_strHostName.c_str(), m_strID.c_str(), pswd_decrypted.c_str());
	if (m_pConn == NULL)
	{
		g_pIOCPEngine->dbgprint("mysql connect failed\n");
		return FALSE;
	}

	//m_pConn->setClientOption("characterSetResults", "utf8");
	//m_pConn->setClientOption("OPT_CHARSET_NAME", "utf8");
	//m_pConn->setClientOption("OPT_SET_CHARSET_NAME", "utf8");

	m_pConn->setSchema(m_strSchema);

	if (pFunc != NULL)
	{
		m_listFuncDBPrepare.push_back(pFunc);
		BOOL ret = pFunc(this);
		if (!ret)
		{
			g_pIOCPEngine->dbgprint("PrepareCommands Failed - %s\n", m_strHostName.c_str());
			return FALSE;
		}
	}
	
	return TRUE;
}

void	THMySQLConnector::Destroy()
{
	if (m_pConn)
	{
		m_pConn->close();
		delete m_pConn;
		m_pConn = NULL;
	}
}

void	THMySQLConnector::Update()
{
	FixConnection();
}

void	THMySQLConnector::FixConnection()
{
	BOOL bNeedConn = FALSE;
	if (m_pConn)
	{
		if (m_pConn->isValid() == false)
		{
			bNeedConn = TRUE;
		}
	}

	if (bNeedConn)
	{
		m_pConn->reconnect();

		if (m_pConn->isValid())
		{
			std::list< PxDBPrepareFunc>::iterator	itor_func = m_listFuncDBPrepare.begin();
			while (itor_func != m_listFuncDBPrepare.end())
			{
				PxDBPrepareFunc	pFunc = *itor_func;

				BOOL ret = pFunc(this);
				if (!ret)
				{
					g_pIOCPEngine->dbgprint("[2] PrepareCommands Failed - %s\n", m_strHostName.c_str());
				}

				++itor_func;
			}
		}
	}
}

void	THMySQLConnector::StartDBTransaction()
{
	m_pConn->setAutoCommit(false);									// Start DB Transaction
}

void	THMySQLConnector::EndDBTransaction(BOOL bCommit)
{
	try
	{
		if (bCommit)
		{
			m_pConn->commit();										// Commit DB Transaction
		}
		else
		{
			m_pConn->setAutoCommit(true);
		}
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_transaction_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "EndDBTransaction:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();
	}
}

void	THMySQLConnector::Rollback(BOOL isSetAutoCommit)
{
	try
	{
		m_pConn->rollback();

		if (isSetAutoCommit)
		{
			m_pConn->setAutoCommit(true);
		}
	}
	catch (sql::SQLException& e)
	{
		auto t = std::chrono::system_clock::now();
		std::time_t tt = std::chrono::system_clock::to_time_t(t);

		std::string		fname = "Report\\mysql_transaction_err.txt";
		std::ofstream	fout;
		fout.open(fname.c_str(), std::ios::out | std::ios::app);
		fout << "EndDBTransaction:" << std::ctime(&tt) << std::endl;
		fout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ")" << std::endl;
		fout << "# ERR: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		fout.close();
	}
}

BOOL	THMySQLConnector::ReadOptionFromJson(int curIndex)
{
	std::string fpath = std::string("../Data_Setting/ServerSetting.json");
	std::wstring	rbuff;
	rapidjson::GenericDocument<rapidjson::UTF16<> > json;
	bool bSuccess = CMReadFile(fpath.c_str(), rbuff);

	if (bSuccess == false)
	{
		fprintf(stderr, "ReadOptionFromJson File Open Error\n");
		return FALSE;
	}

	json.Parse<0>(rbuff.c_str());

	if (json.IsArray())
	{
		for (rapidjson::SizeType i = 0; i < json.Size(); i++)
		{
			if (json[i].IsObject())
			{
				int index = -1;
				if (json[i].HasMember(L"index"))	index = json[i][L"index"].GetInt();
				if (index == curIndex)
				{
					if (json[i].HasMember(L"serverIPDBTable"))
					{
						m_strServerIPTable = ConvertWStrToStr(json[i][L"serverIPDBTable"].GetString());
					}

					if (json[i].HasMember(L"mySQL_HostName"))
					{
						m_strHostName = ConvertWStrToStr(json[i][L"mySQL_HostName"].GetString());
					}

					if (json[i].HasMember(L"mySQL_Port"))	m_iPort = (UINT16)json[i][L"mySQL_Port"].GetUint();

					if (json[i].HasMember(L"mySQL_ID"))
					{
						m_strID = ConvertWStrToStr(json[i][L"mySQL_ID"].GetString());
					}

					if (json[i].HasMember(L"mySQL_PSWD"))
					{
						m_strPSWD_Encrypted = ConvertWStrToStr(json[i][L"mySQL_PSWD"].GetString());
					}

					if (json[i].HasMember(L"mySQL_Schema"))
					{
						m_strSchema = ConvertWStrToStr(json[i][L"mySQL_Schema"].GetString());
					}
				}
			}
		}
	}

	return TRUE;
}


