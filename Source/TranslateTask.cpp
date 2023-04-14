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
			int type = *(iterator++);
			std::string type_name = ((type < version->types_code_names.size()) ?
				version->types_code_names[type] :
				"s" + (std::to_string(type - version->types_code_names.size())));

			std::string str = "out " + type_name +
				" p" + std::to_string(temp->SentCounter) + ';';
			temp->SentCounter++;
			temp->sent_vars_types.push_back(type);
			write_target->insert(write_target->begin(), str.begin(), str.end());
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			break;
		}			
		case binary_to_glsl_conversion_exception::Catch:
		{
			std::string type_name = ((*iterator < version->types_code_names.size()) ?
				version->types_code_names[*iterator] :
				"s" + (std::to_string(*iterator - version->types_code_names.size())));

			temp->is_struct.push_back(*iterator >= version->types_code_names.size());
			std::string str = "in " + type_name + " p" + std::to_string(*(iterator + 1)) + ';';

			iterator += 2;
			write_target->insert(write_target->begin(), str.begin(), str.end());
			break;
		}		
		case binary_to_glsl_conversion_exception::FunctionHeader:
		{
			exception_result.push_back('(');
			int args_count = *iterator;
			temp->function_args_types.push_back({});
			for (int i = 0; i < args_count; i++)
			{
				if (i != 0)
					exception_result.push_back(',');
				++iterator;
				std::string arg = ((*iterator < version->types_code_names.size()) ?
					version->types_code_names[*iterator] :
					"s" + (std::to_string(*iterator - version->types_code_names.size())))
					+ " v" + std::to_string(i + temp->vars_at_id.back());

				ReturnType r_type;
				switch (*iterator)
				{
				case 0:  r_type = ReturnType::Int;   break;
				case 1:  r_type = ReturnType::Float; break;
				default: r_type = ReturnType::Other; break;
				}

				temp->function_args_types.back().push_back(r_type);
				temp->temp_vars_types.push_back(*iterator);
				temp->is_struct.push_back(*iterator >= version->types_code_names.size());
				exception_result.insert(exception_result.end(), arg.begin(), arg.end());
			}
			exception_result.push_back(')');
			exception_result.push_back('{');
			temp->function_args_count.push_back(args_count);
			++iterator;
			break;
		}
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