#include "StandardVersion.h"

#include "../Standards/S0/S0.h"

std::unique_ptr<StandardVersion> CreateStandardVersion(unsigned int version)
{
	std::unique_ptr<StandardVersion> V;
	switch (version)
	{
	case 0:
		V = Standards::S0Create();
		break;
	default:
		V = nullptr;
	}
	return V;
}