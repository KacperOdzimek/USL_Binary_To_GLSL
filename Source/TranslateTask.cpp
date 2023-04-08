#include "TranslationTask.h"
#include "StandardVersion.h"
#include "LoadMathExpression.h"

void TranslationTask::Start(void* source, int size)
{
	temp = new Temp;

	std::unique_ptr<StandardVersion> version = CreateStandardVersion(0);
	uint8_t* iterator = (uint8_t*)source;
	std::string glsl_version = "#version 330\n";

	std::vector<uint8_t> common{ glsl_version.begin(), glsl_version.end() };
	std::vector<uint8_t> vertex{};
	std::vector<uint8_t> fragment{};

	std::vector<uint8_t>* write_target = &common;

	while (iterator - source < size)
	{
		auto& current = version->glsl_signatures_alternatives.at(*iterator);
		std::vector<uint8_t> args{ iterator + 1, iterator + current.arguments + 1 };

		auto converted = current.func(args, temp);

		switch (temp->write_target)
		{
		case Temp::WriteTarget::Common:
			write_target = &common; break;
		case Temp::WriteTarget::Vertex:
			write_target = &vertex; break;
		case Temp::WriteTarget::Fragment:
			write_target = &fragment; break;
		}	

		write_target->insert(write_target->end(), converted.second.begin(), converted.second.end());

		iterator = iterator + current.arguments + 1;

		std::vector<uint8_t> exception_result;
		switch (converted.first)
		{
		case binary_to_glsl_conversion_exception::FatalError:
			return; break;
		case binary_to_glsl_conversion_exception::LoadLiteral:
			exception_result = version->literal_decoding_functions.at(*(iterator - 1)).func(iterator);
			iterator += version->literal_decoding_functions.at(*(iterator - 1)).literal_size;
			break;
		case binary_to_glsl_conversion_exception::LoadMathExpression:
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			break;
		case binary_to_glsl_conversion_exception::Send:
		{
			//+1
			std::string str = "out " + version->types_code_names[*(++iterator)] + 
				" p" + std::to_string(temp->SentCounter) + ';';
			temp->SentCounter++;
			write_target->insert(write_target->begin(), str.begin(), str.end());
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			break;
		}			
		case binary_to_glsl_conversion_exception::Catch:
			std::string str = "in " + version->types_code_names[*(++iterator)] +
				" p" + std::to_string(*(++iterator)) + ';';
			write_target->insert(write_target->begin(), str.begin(), str.end());
			break;
		}

		write_target->insert(write_target->end(), exception_result.begin(), exception_result.end());

		if (current.add_semicolon)
			write_target->push_back(';');
	}

	if (temp->deepness != 0)
		for (int i = 0; i < temp->deepness; i++)
			write_target->push_back('}');

	delete temp;

	result.success = true;
	result.data.reserve(common.size() + vertex.size() + fragment.size());
	result.data.insert(result.data.end(), common.begin(),   common.end());
	result.data.push_back('\n');
	result.data.insert(result.data.end(), vertex.begin(),   vertex.end());
	result.data.push_back('\n');
	result.data.insert(result.data.end(), fragment.begin(), fragment.end());
}