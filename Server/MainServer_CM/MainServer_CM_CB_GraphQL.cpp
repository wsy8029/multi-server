#include "MainServer_CM.h"

void	MainServer_CM::InitGraphQLCBFunction()
{
	g_csGraphQLManager.SetGraphQLCompleteCB(NestoQL::EGQLMT_SignIn, ProcGQLSignIn);
	g_csGraphQLManager.SetGraphQLCompleteCB(NestoQL::EGQLMT_MyProfile, ProcGQLMyProfile);

}

BOOL	MainServer_CM::ProcGQLSignIn (stGraphQLJob* pJob)
{
	if (pJob->result == NestoQL::GQR_SUCCESS)
	{
		NestoQL::SignInOutput	SIO;
		SIO.Deserialize(pJob->resultJson);
	}
	else if (pJob->result == NestoQL::GQR_ERROR)
	{
		NestoQL::ResultError	err;
		err.Deserialize(pJob->resultJson);
	}

	g_csGraphQLManager.m_pPoolSignInInput->Free((NestoQL::SignInInput*)pJob->pInput);

	return TRUE;
}

BOOL	MainServer_CM::ProcGQLMyProfile(stGraphQLJob* pJob)
{
	stClientSession* pSession = (stClientSession*)m_pThis->m_csAdminClientSession.GetObjectByKey(pJob->GUID);
	if (pSession != NULL)																		// 유저에게 접속 실패를 알린다.. 해당 guid가 없다
	{
		PxWSBuffer* pToClient = g_pWSEngine->GetSendBufferObj(pSession->pWSSession);
		if (pToClient != nullptr)
		{
			if (pJob->result == NestoQL::GQR_SUCCESS)
			{
				NestoQL::MyProfileOutput	MPO;
				MPO.Deserialize(pJob->resultJson);

				stAccount* pAC = &pSession->account;

				UINT64 CHGUID = MPO.id;
				stPlayerCharacter* pPC = m_pThis->m_pPoolPlayerCharacter->Allocate();
				pPC->Init();
				pPC->TID = 1;
				pPC->pPCT = (stPlayerCharacterTemplate*)g_csTemplateManager.m_csAdminPlayerCharacterTemplate.GetObjectByKey(pPC->TID);
				stCharacterInfo* pCHInfo = (stCharacterInfo*)m_pThis->m_csAdminCharacterInfo.GetObjectByKey(CHGUID);
				if (pCHInfo == NULL)
				{
					pCHInfo = m_pThis->m_pPoolCharacterInfo->Allocate();
					pCHInfo->Init();
					pCHInfo->CHGUID = CHGUID;

					m_pThis->m_listCharacterInfo.push_back(pCHInfo);
					m_pThis->m_csAdminCharacterInfo.AddObject(pCHInfo, pCHInfo->CHGUID);
				}
				pPC->CHGUID = pAC->CHGUID = CHGUID;
				pPC->ACGUID = pAC->ACGUID;
				pPC->pCharacterInfo = pCHInfo;
				pSession->pPC = pPC;

				m_pThis->DBGet_Auth(pSession);

				m_pThis->InitPlayerCharacterByMyProfile(pPC, MPO);

				pToClient->WriteUI8(0);					// Success

				UINT16 accountItemCount = (UINT16)pAC->listAccountItem.size();
				pToClient->WriteUI16(accountItemCount);
				std::list<stAccountItem*>::iterator itor_acitem = pAC->listAccountItem.begin();
				while (itor_acitem != pAC->listAccountItem.end())
				{
					stAccountItem* pACItem = *itor_acitem;

					pToClient->WriteUI64(pACItem->itemGUID.productID);
					pToClient->WriteUI16(pACItem->count);
					++itor_acitem;
				}

				// PC
				pToClient->WriteUI64(CHGUID);
				pToClient->WriteUI16(pPC->TID);

				pToClient->WriteString(pCHInfo->nickname_UTF8);
				for (int i = 0; i < ECCT_Count; ++i)
				{
					pToClient->WriteUI64(pCHInfo->customize[i]);
				}

				pToClient->WriteUI8(EIEST_Count);
				for (int i = 0; i < EIEST_Count; ++i)
				{
					if (pPC->equipItem[i] == NULL)
					{
						pToClient->WriteUI8(0);
					}
					else
					{
						pToClient->WriteUI8(1);

						stItem* pItem = pPC->equipItem[i];
						pToClient->WriteUI64(pItem->itemGUID.productID);
					}
				}

				UINT16 characterItemCount = (UINT16)pPC->listCharacterItem.size();
				pToClient->WriteUI16(characterItemCount);
				std::list<stCharacterItem*>::iterator itor_chitem = pPC->listCharacterItem.begin();
				while (itor_chitem != pPC->listCharacterItem.end())
				{
					stCharacterItem* pCHItem = *itor_chitem;

					pToClient->WriteUI64(pCHItem->itemGUID.productID);
					pToClient->WriteUI16(pCHItem->count);
					++itor_chitem;
				}

				pToClient->WriteUI32(pPC->mapTID);									// 맵 미리 로딩하기 위해서..
				g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_MAIN_SERVER_AUTH);
			}
			else if (pJob->result == NestoQL::GQR_ERROR)
			{
				NestoQL::ResultError	err;
				err.Deserialize(pJob->resultJson);

				pToClient->WriteUI8(3);					// GraphQL Error
				if (err.errorsCount > 0)
				{
					pToClient->WriteI32(err.errors[0].extensions.code);
				}
				else
				{
					pToClient->WriteI32(0);
				}
			}
			else if (pJob->result == NestoQL::GQR_CURL_ERROR)
			{
				pToClient->WriteUI8(4);					// CURL Error
			}

			g_pWSEngine->SendBuffer(pToClient, PK_CONNECT_MAIN_SERVER_AUTH);
		}
		return FALSE;
	}

	return TRUE;
}
