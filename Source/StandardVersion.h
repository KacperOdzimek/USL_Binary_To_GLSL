#pragma once
#include <vector>
#include <functional>

#include "TranslationTask.h"

enum class binary_to_glsl_conversion_exception : uint8_t
{
	None, FatalError, LoadLiteral, LoadMathExpression, Send, Catch, FunctionHeader, ArrayLiteral, GeometryShaderReturnCall
};

class StandardVersion
{
public:
	using translate_signature_func = std::function<std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		(std::vector<uint8_t>&, Temp*)>;
	using literal_decoding_func = std::function<std::vector<uint8_t>(uint8_t* ptr)>;

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

	struct literal_decoding_function
	{
		//in bytes
		int literal_size;
		literal_decoding_func func;
		literal_decoding_function(int _literal_size, literal_decoding_func _func) :
			literal_size(_literal_size), func(_func) {};
	};

	//names of vars in glsl, position on vector = id in usl
	std::vector <std::string> types_code_names;

	std::vector<literal_decoding_function> literal_decoding_functions;
};

std::unique_ptr<StandardVersion> CreateStandardVersion(unsigned int version);