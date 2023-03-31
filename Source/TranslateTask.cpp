#include "TranslateTask.h"
#include "StandardVersion.h"

void TranslateTask::Start(void* source, int size)
{
	std::unique_ptr<StandardVersion> version = CreateStandardVersion(0);
	uint8_t* iterator = (uint8_t*)source;
	std::vector<uint8_t> glsl;

	while (iterator - source < size)
	{
		auto& current = version->glsl_signatures_alternatives.at(*iterator);
		std::vector<uint8_t> args;
		for (int i = 0; i < current.first; i++)
			args.push_back(*(iterator++));
		auto converted = current.second(args);
		glsl.insert(glsl.end(), converted.begin(), converted.end());
		glsl.push_back('\n');
		++iterator;
	}

	result.success = true;
	result.data = glsl;
}