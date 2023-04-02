#include "S0.h"

std::unique_ptr<StandardVersion> Standards::S0Create()
{
	std::unique_ptr<StandardVersion> version = std::make_unique<StandardVersion>();

#define Insert(text) [](std::vector<uint8_t>& in, Temp* temp)												\
	-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>									\
{																											\
		std::string str = text;																				\
		return { binary_to_glsl_conversion_exception::None, std::vector<char>(str.begin(), str.end()) };	\
}																											\

	//End of line
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->deepness -= 1;
			if (temp->deepness == 0)
				temp->context = Temp::Context::GlobalScope;

			std::string str = "}";
			return { binary_to_glsl_conversion_exception::None, std::vector<char>(str.begin(), str.end()) };
		}
	});

	//VertexShader
	version->glsl_signatures_alternatives.push_back({ false, 0, 
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->context = Temp::Context::Shader;
			temp->shader_type = Temp::ShaderType::VertexShader;
			temp->deepness = 1;

			std::string str = "void main() {";
			return { binary_to_glsl_conversion_exception::None, std::vector<char>(str.begin(), str.end()) };
		}
	});

	//PixelShader
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->context = Temp::Context::Shader;
			temp->shader_type = Temp::ShaderType::PixelShader;
			temp->deepness = 1;

			std::string str = "void main() {";
			return { binary_to_glsl_conversion_exception::None, std::vector<char>(str.begin(), str.end()) };
		}
	});

	version->glsl_signatures_alternatives.push_back({ true, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str;
			switch (temp->shader_type)
			{
			case Temp::ShaderType::VertexShader:
				str = "gl_Position = "; break;
			case Temp::ShaderType::PixelShader:
				str = "FragColor = "; break;
			} 
			return { binary_to_glsl_conversion_exception::LoadMathExpression, std::vector<char>(str.begin(), str.end()) };
		}
	});

	version->glsl_signatures_alternatives.push_back({ false, 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ false, 0, Insert("void main() {") });

	version->glsl_signatures_alternatives.push_back({ false, 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ false, 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ false, 0, Insert("void main() {") });

	version->glsl_signatures_alternatives.push_back({ false, 1, Insert("layout (location = 0) in vec3 aPos;")});

#undef Insert

	return version;
}