#include "../Include/USL_Binary_To_GLSL.h"
#include "TranslationTask.h"
#include "StandardVersion.h"
#include "LoadMathExpression.h"

#include <iostream>

void TranslationTask::Start(void* source, int size, Temp* owning_task_temp)
{
	temp = new Temp;

	std::unique_ptr<StandardVersion> version = CreateStandardVersion(0);
	uint8_t* iterator = (uint8_t*)source;
	uint8_t std_version = *(iterator);
	uint8_t file_type = *(++iterator);

	switch (file_type)
	{
	case 0:
		temp->file_type = Temp::FileType::Shader; 
		iterator++;
		break;
	case 1:
	{
		temp->file_type = Temp::FileType::Library;
		//Skip functions headers positions
		uint16_t headers_position;
		uint8_t arr[2] = {*(++iterator), *(++iterator)};
		memcpy(&headers_position, &arr[0], 2);

		iterator += headers_position * 2 + 2;

		break;
	}
	case 2:
		temp->file_type = Temp::FileType::Metashader; break;
	}

	if (owning_task_temp != nullptr && temp->file_type == Temp::FileType::Library)
	{
		temp->FuncCounter = owning_task_temp->FuncCounter;
		temp->variable_id_shift_value = owning_task_temp->vars_at_id.at(0);
		for (auto& ipv : owning_task_temp->cached_imported_vars)
			temp->variable_id_shift_value += ipv.count;
	}

	std::vector<uint8_t> common;
	std::string glsl_version = "#version 330\n";

	if (temp->file_type == Temp::FileType::Shader)
		common.insert(common.end(), glsl_version.begin(), glsl_version.end());

	std::vector<uint8_t> vertex{};
	std::vector<uint8_t> fragment{};
	std::vector<uint8_t> geometry{};

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
		case Temp::WriteTarget::Geometry:
			write_target = &geometry; break;
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
			if (temp->file_type == Temp::FileType::Library)
			{
				int name_size = *(iterator);
				iterator += name_size + 1;
				break;
			}
			break;
		case binary_to_glsl_conversion_exception::LoadMathExpression:
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			if (temp->file_type == Temp::FileType::Library)
			{
				int name_size = *(iterator);
				iterator += name_size + 1;
				break;
			}
			break;
		case binary_to_glsl_conversion_exception::Send:
		{
			if (true)
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
			}
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			break;
		}			
		case binary_to_glsl_conversion_exception::Catch:
		{
			std::string type_name = ((*iterator < version->types_code_names.size()) ?
				version->types_code_names[*iterator] :
				"s" + (std::to_string(*iterator - version->types_code_names.size())));

			temp->is_struct.push_back(*iterator >= version->types_code_names.size());
			std::string str = "in " + type_name + " p" + std::to_string(*(iterator + 1));

			if (temp->shader_type == Temp::ShaderType::GeometryShader)
				str += "[]";

			str += ';';

			temp->temp_vars_types.push_back(*iterator);
			temp->is_struct.push_back(0);

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

			//skip function name
			if (temp->file_type == Temp::FileType::Library)
			{
				int name_size = *(iterator);
				iterator += name_size + 1;
			}
				
			break;
		}
		case binary_to_glsl_conversion_exception::ArrayLiteral:
		{
			std::vector<uint8_t> size_as_bin = { *(iterator - 1), *(iterator - 2), *(iterator - 3), *(iterator - 4) };
			int arr_size; memcpy(&arr_size, &(*size_as_bin.begin()), 4);

			exception_result.push_back('(');
			for (int i = 0; i < arr_size; i++)
			{
				auto r = LoadMathExpression(iterator, version.get(), temp);
				exception_result.insert(exception_result.end(), r.begin(), r.end());
				if (i + 1 != arr_size)
					exception_result.push_back(',');
			}
			exception_result.push_back(')');
			break;
		}
		case binary_to_glsl_conversion_exception::GeometryShaderReturnCall:
		{
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			std::string str = ";EmitVertex()";
			exception_result.insert(exception_result.end(), str.begin(), str.end());
			break;
		}
		case binary_to_glsl_conversion_exception::SendOverwrite:
		{
			iterator++;	//bypass type
			exception_result = LoadMathExpression(iterator, version.get(), temp);
			uint8_t overwriten_send = *(iterator++);

			std::string name = 'p' + std::to_string(overwriten_send) + '=';
			exception_result.insert(exception_result.begin(), name.begin(), name.end());

			break;
		}
		case binary_to_glsl_conversion_exception::UsingLibrary:
		{
			int lib_name_size = *(iterator++);
			std::string lib_name;
			lib_name.insert(lib_name.begin(), (char*)iterator, (char*)iterator + lib_name_size);
			iterator += lib_name_size;

			auto content = USL_Translator::USL_Binary_To_GLSL::LEFC(0, lib_name);
			if (content.size != 0)
			{
				uint8_t* lib_iterator = (uint8_t*)content.position;
				TranslationTask translate_task;
				translate_task.Start(lib_iterator, content.size, temp);
				auto& x = translate_task.result;
				exception_result.insert(exception_result.begin(), x.data.begin(), x.data.end());
			}
			break;
		}
		case binary_to_glsl_conversion_exception::SkipName:
		{
			int name_size = *(iterator);
			iterator += name_size + 1;
			break;
		}
		case binary_to_glsl_conversion_exception::MacroCaseWithExp:
		{
			int exp_size = *(iterator);
			decltype(iterator) exp_begin = iterator;
			iterator += exp_size + 1;

			std::string _case;
			int name_size = *(iterator++);
			_case.insert(_case.end(), iterator, iterator + name_size);

			if (_case == "GLSL")
			{
				exception_result = LoadMathExpression(exp_begin, version.get(), temp);
				exception_result.push_back(';');
			}

			iterator += name_size;

			break;
		}
		case binary_to_glsl_conversion_exception::MacroCaseWithText:
		{
			std::string _case;
			int name_size = *(iterator++);
			_case.insert(_case.end(), iterator, iterator + name_size);

			std::string content;
			iterator += name_size;
			int content_size = *(iterator++);
			content.insert(content.end(), iterator, iterator + content_size);

			if (_case == "GLSL")
			{
				exception_result.insert(exception_result.end(), content.begin(), content.end());
				exception_result.push_back(';');
			}

			iterator += content_size;
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

	if (owning_task_temp != nullptr && temp->file_type == Temp::FileType::Library)
	{
		owning_task_temp->function_args_count.insert(owning_task_temp->function_args_count.end(),
			temp->function_args_count.begin(), temp->function_args_count.end());

		owning_task_temp->function_args_types.insert(owning_task_temp->function_args_types.end(),
			temp->function_args_types.begin(), temp->function_args_types.end());

		owning_task_temp->function_return_types.insert(owning_task_temp->function_return_types.end(),
			temp->function_return_types.begin(), temp->function_return_types.end());

		owning_task_temp->structs.insert(owning_task_temp->structs.end(),
			temp->structs.begin(), temp->structs.end());

		if (owning_task_temp->vertex_layout_type != -1)
		{
			owning_task_temp->vars_at_id.at(0) += temp->vars_at_id.at(0);
			owning_task_temp->is_struct.insert(owning_task_temp->is_struct.end(), temp->is_struct.begin(), temp->is_struct.end());
			owning_task_temp->temp_vars_types.insert(owning_task_temp->temp_vars_types.end(), temp->temp_vars_types.begin(), temp->temp_vars_types.end());
		}
		else
		{
			Temp::ImportedVarsPackage ivp;
			ivp.count = temp->vars_at_id.at(0);
			ivp.is_struct.insert(ivp.is_struct.end(), temp->is_struct.begin(), temp->is_struct.end());
			ivp.types.insert(ivp.types.end(), temp->temp_vars_types.begin(), temp->temp_vars_types.end());
			owning_task_temp->cached_imported_vars.push_back(ivp);
		}

		owning_task_temp->FuncCounter = temp->FuncCounter;
	}

	if (temp->file_type == Temp::FileType::Shader)
	{
		result.success = true;
		result.data.reserve(common.size() + vertex.size() + geometry.size() + fragment.size());
		result.data.insert(result.data.end(), common.begin(), common.end());
		result.data.push_back('\n');
		result.data.insert(result.data.end(), vertex.begin(), vertex.end());
		result.data.push_back('\n');
		if (geometry.size() != 0)
		{
			result.data.insert(result.data.end(), geometry.begin(), geometry.end());
			result.data.push_back('\n');
		}
		result.data.insert(result.data.end(), fragment.begin(), fragment.end());
	}
	else
	{
		result.success = true;
		result.data = common;
	}

	delete temp;
}