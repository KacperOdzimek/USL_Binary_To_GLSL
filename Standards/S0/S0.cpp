#include "S0.h"

std::unique_ptr<StandardVersion> Standards::S0Create()
{
	std::unique_ptr<StandardVersion> version = std::make_unique<StandardVersion>();

#define Insert(text) [](std::vector<uint8_t>& in)					\
{																	\
		std::string str = text;									\
		return std::vector<char>(str.begin(), str.end());		\
}																	\

	//End of line
	version->glsl_signatures_alternatives.push_back({0, Insert( "}") });
	version->glsl_signatures_alternatives.push_back({0, Insert( "void main() {" ) });
	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });

	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });

	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });
	version->glsl_signatures_alternatives.push_back({ 0, Insert("void main() {") });

	version->glsl_signatures_alternatives.push_back({ 1, Insert("layout (location = 0) in vec3 aPos;")});

#undef Insert

	return version;
}