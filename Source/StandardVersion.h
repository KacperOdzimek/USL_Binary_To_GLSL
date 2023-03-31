#pragma once
#include <vector>
#include <functional>

class StandardVersion
{
public:
	using translate_signature_func = std::function < std::vector<char>(std::vector<uint8_t>&) > ;

	//arguments size (in bytes), binary to glsl convertiong function
	std::vector<std::pair<int, translate_signature_func>> glsl_signatures_alternatives;
};

std::unique_ptr<StandardVersion> CreateStandardVersion(unsigned int version);