#include "TranslationTask.h"
#include "StandardVersion.h"
#include "LoadMathExpression.h"

void TranslationTask::Start(void* source, int size)
{
	temp = new Temp;

	std::unique_ptr<StandardVersion> version = CreateStandardVersion(0);
	uint8_t* iterator = (uint8_t*)source;
	std::vector<uint8_t> glsl;

	while (iterator - source < size)
	{
		auto& current = version->glsl_signatures_alternatives.at(*iterator);
		std::vector<uint8_t> args;
		for (int i = 0; i < current.arguments; i++)
			args.push_back(*(iterator++));

		auto converted = current.func(args, temp);
		++iterator;

		glsl.insert(glsl.end(), converted.second.begin(), converted.second.end());

		switch (converted.first)
		{
		case binary_to_glsl_conversion_exception::FatalError:
			return; break;
		case binary_to_glsl_conversion_exception::LoadMathExpression:
			auto r = LoadMathExpression(iterator);
			glsl.insert(glsl.end(), r.begin(), r.end());
			break;
		}
	
		glsl.push_back('\n');
	}

	if (temp->deepness != 0)
		for (int i = 0; i < temp->deepness; i++)
			glsl.push_back('}');

	delete temp;

	result.success = true;
	result.data = glsl;
}