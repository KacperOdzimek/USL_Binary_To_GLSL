#pragma once
#include <vector>
#include <functional>

#include "TranslationTask.h"

enum class binary_to_glsl_conversion_exception : uint8_t
{
	None, FatalError, LoadMathExpression
};

class StandardVersion
{
public:
	using translate_signature_func = std::function <std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		(std::vector<uint8_t>&, Temp*)>;

	//arguments size (in bytes), binary to glsl conversion function

	struct signature
	{
		//whether to add semicolon or not
		bool add_semicolon;
		//in bytes
		int arguments;
		//binary to glsl conversion function
		translate_signature_func func;
		
		signature(bool _add_semicolon, int _arguments, translate_signature_func _func) :
			add_semicolon(_add_semicolon), arguments(_arguments), func(_func) {};
	};

	std::vector<signature> glsl_signatures_alternatives;
};

std::unique_ptr<StandardVersion> CreateStandardVersion(unsigned int version);