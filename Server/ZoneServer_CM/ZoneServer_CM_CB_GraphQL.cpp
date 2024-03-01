#include "ZoneServer_CM.h"

void	ZoneServer_CM::InitGraphQLCBFunction()
{
	g_csGraphQLManager.SetGraphQLCompleteCB(NestoQL::EGQLMT_SignIn, ProcGQLSignIn);

}

BOOL	ZoneServer_CM::ProcGQLSignIn(stGraphQLJob* pJob)
{
	if (pJob->result == NestoQL::GQR_SUCCESS)
	{
		NestoQL::SignInOutput	SIO;
		SIO.Deserialize(pJob->resultJson);
		int a = 0;
	}
	else if (pJob->result == NestoQL::GQR_ERROR)
	{
		NestoQL::ResultError	err;
		err.Deserialize(pJob->resultJson);
		int a = 0;
	}

	return TRUE;
}

