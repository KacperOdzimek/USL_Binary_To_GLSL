#include "S0.h"

std::string decode_float(uint8_t* ptr)
{
	std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
	float f;
	memcpy(&f, &(*as_bin.begin()), 4);

	std::string str = std::to_string(f);
	str.erase(str.find_last_not_of('0') + 1, std::string::npos);
	str.erase(str.find_last_not_of('.') + 1, std::string::npos);

	return str;
}

std::string decode_int(uint8_t* ptr)
{
	std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
	int i;
	memcpy(&i, &(*as_bin.begin()), 4);

	std::string str = std::to_string(i);

	return str;
}

std::string gen_geometry_output_layout_line(Temp*& temp)
{
	std::string str;
	if (temp->geometry_shader_output_primitive_id != -1)
	{
		str += "layout(";
		switch (temp->geometry_shader_output_primitive_id)
		{
		case 0: str += "points"; break;
		case 1: str += "line_strip"; break;
		case 2: str += "triangle_strip"; break;
		}
		str += ", max_vertices = ";
		str += std::to_string(temp->geometry_shader_vertices_limit);
		str += ") out;";
	}
	return str;
}

std::unique_ptr<StandardVersion> Standards::S0Create()
{
	StandardVersion* version = new StandardVersion;

	version->types_code_names = {"int", "float", "vec2", "vec3", "vec4", "array_error"};

	version->built_in_functions.push_back({ "distance", 0, {ReturnType::Int, ReturnType::Int} });
	version->built_in_functions.push_back({ "distance", 1, {ReturnType::Float, ReturnType::Float} });
	version->built_in_functions.push_back({ "distance", 2, {ReturnType::Other, ReturnType::Other} });
	version->built_in_functions.push_back({ "distance", 3, {ReturnType::Other, ReturnType::Other} });
	version->built_in_functions.push_back({ "distance", 4, {ReturnType::Other, ReturnType::Other} });

	version->built_in_functions.push_back({ "round", 0, {ReturnType::Float} });
	version->built_in_functions.push_back({ "cell",  0, {ReturnType::Float} });
	version->built_in_functions.push_back({ "floor", 0, {ReturnType::Float} });

	version->built_in_functions.push_back({ "sqrt",  1, {ReturnType::Float} });

#define Insert(text) [](std::vector<uint8_t>& in, Temp* temp)													\
	-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>										\
	{																											\
			std::string str = text;																				\
			return { binary_to_glsl_conversion_exception::None, std::vector<char>(str.begin(), str.end()) };	\
	}																											\
	//Int
	version->literal_decoding_functions.push_back({ 4, 
		[](uint8_t* ptr) -> std::vector<uint8_t>
		{
			std::string as_str = decode_int(ptr);
			return { as_str.begin(), as_str.end() };
		}
	});

	//Float
	version->literal_decoding_functions.push_back({ 4,
		[](uint8_t* ptr) -> std::vector<uint8_t>
		{
			std::string as_str = decode_float(ptr);
			return { as_str.begin(), as_str.end() };
		}
	});

	//Vec2
	version->literal_decoding_functions.push_back({ 8,
		[](uint8_t* ptr) -> std::vector<uint8_t>
		{
			std::string value1 = decode_float(ptr);
			std::string value2 = decode_float(ptr + 4);
			std::string as_str = "vec2(" + 
				value1 + "," + 
				value2 + ")";
			return { as_str.begin(), as_str.end() };
		}
	});

	//Vec3
	version->literal_decoding_functions.push_back({ 12,
		[](uint8_t* ptr) -> std::vector<uint8_t>
		{
			std::string value1 = decode_float(ptr);
			std::string value2 = decode_float(ptr + 4);
			std::string value3 = decode_float(ptr + 8);
			std::string as_str = "vec3(" +
				value1 + "," +
				value2 + "," +
				value3 + ")";
			return { as_str.begin(), as_str.end() };
		}
	});

	//Vec4
	version->literal_decoding_functions.push_back({ 16,
		[](uint8_t* ptr) -> std::vector<uint8_t>
		{
			std::string value1 = decode_float(ptr);
			std::string value2 = decode_float(ptr + 4);
			std::string value3 = decode_float(ptr + 8);
			std::string value4 = decode_float(ptr + 12);
			std::string as_str = "vec4(" +
				value1 + "," +
				value2 + "," +
				value3 + "," +
				value4 + ")";
			return { as_str.begin(), as_str.end() };
		}
		});

	//End of block
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str = (temp->context == Temp::Context::StructDeclaration) ? "};" : "}";

			temp->vars_at_id.erase(temp->vars_at_id.end() - 1);
			temp->is_struct.resize(temp->vars_at_id.back());

			temp->deepness -= 1;
			if (temp->deepness == 0)
			{
				temp->temp_vars_types.clear();
				temp->context = Temp::Context::GlobalScope;
			}

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//VertexShader
	version->glsl_signatures_alternatives.push_back({ false, 0, 
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->context = Temp::Context::Shader;
			temp->shader_type = Temp::ShaderType::VertexShader;
			temp->write_target = Temp::WriteTarget::Vertex;
			temp->deepness = 1;
			temp->vars_at_id.push_back(temp->vars_at_id.back());

			//Vertex components cannot be struct
			for (int i = 0; i < temp->vars_at_id.back(); i++)
				temp->is_struct.push_back(false);

			std::string str = "void main() {";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//PixelShader
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->context = Temp::Context::Shader;
			temp->shader_type = Temp::ShaderType::PixelShader;
			temp->write_target = Temp::WriteTarget::Fragment;
			temp->deepness = 1;
			temp->vars_at_id.push_back(temp->vars_at_id.back());

			std::string str = "out vec4 FragColor; void main() {";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//GeometryShader
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->context = Temp::Context::Shader;
			temp->shader_type = Temp::ShaderType::GeometryShader;
			temp->write_target = Temp::WriteTarget::Geometry;
			temp->deepness = 1;
			temp->vars_at_id.push_back(temp->vars_at_id.back());

			temp->does_array_contain_struct.insert({ temp->vars_at_id.back(), false});
			temp->vars_at_id.back()++;
			temp->temp_vars_types.push_back(6);

			std::string str = "void main() {";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//return ?e
	version->glsl_signatures_alternatives.push_back({ true, 0,
		[](std::vector<uint8_t>& in, Temp* temp)
		-> std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str;
			if (temp->context == Temp::Context::CustomFunction)
				str = "return ";
			else
				switch (temp->shader_type)
				{
				case Temp::ShaderType::VertexShader: 
				case Temp::ShaderType::GeometryShader:
					str = "gl_Position = "; break;
				case Temp::ShaderType::PixelShader:
					str = "FragColor = "; break;
				} 
			if (temp->shader_type == Temp::ShaderType::GeometryShader)
				return { binary_to_glsl_conversion_exception::GeometryShaderReturnCall, {str.begin(), str.end()} };
			return { binary_to_glsl_conversion_exception::LoadMathExpression, {str.begin(), str.end()} };
		}
	});

	//?t ?n
	version->glsl_signatures_alternatives.push_back({ true, 1, 
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			if (temp->context == Temp::Context::StructDeclaration)
			{
				std::string str;
				str += version->types_code_names.at(in[0]);
				str += " m" + std::to_string(temp->vars_at_id.back());
				temp->structs.at(temp->structs.size() - 1).push_back(in[0]);
				temp->vars_at_id.back()++;
				return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
			}
			else
			{
				temp->temp_vars_types.push_back(in[0]);
				std::string str;
				str += ((in[0] < version->types_code_names.size()) ?
					version->types_code_names[in[0]] :
					"s" + (std::to_string(in[0] - version->types_code_names.size())));
				str += " v" + std::to_string(temp->vars_at_id.back());
				temp->vars_at_id.back()++;
				temp->is_struct.push_back(in[0] >= version->types_code_names.size());
				return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
			}
		}
	});

	//?t ?n = ?l
	version->glsl_signatures_alternatives.push_back({ true, 1,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->temp_vars_types.push_back(in[0]);
			std::string str;
			str += ((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size())));
			str += " v" + std::to_string(temp->vars_at_id.back()) + '=';
			temp->vars_at_id.back()++;
			temp->is_struct.push_back(in[0] >= version->types_code_names.size());
			return { binary_to_glsl_conversion_exception::LoadLiteral, {str.begin(), str.end()} };
		}
	});

	//?t ?n = ?e
	version->glsl_signatures_alternatives.push_back({ true, 1,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->temp_vars_types.push_back(in[0]);
			std::string str;
			str += ((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size())));
			str += " v" + std::to_string(temp->vars_at_id.back()) + '=';
			temp->vars_at_id.back()++;
			temp->is_struct.push_back(in[0] >= version->types_code_names.size());
			return { binary_to_glsl_conversion_exception::LoadMathExpression, {str.begin(), str.end()} };
		}
	});

	//?t ?n [ ?i ]
	version->glsl_signatures_alternatives.push_back({ true, 5,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str;
			str += ((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size())));
			auto arr_size_as_bin = version->literal_decoding_functions.at(0).func(&in[1]);
			str += " v" + std::to_string(temp->vars_at_id.back()) + '[';
			str.insert(str.end(), arr_size_as_bin.begin(), arr_size_as_bin.end());
			str += ']';

			temp->does_array_contain_struct.insert({ temp->vars_at_id.back(), !(in[0] < version->types_code_names.size()) });
			temp->vars_at_id.back()++;
			temp->temp_vars_types.push_back(6);

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//?t ?n [ ?i ] = ?a
	version->glsl_signatures_alternatives.push_back({ true, 5,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str;
			auto type = ((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size())));
			str += type;
			auto arr_size_as_bin = version->literal_decoding_functions.at(0).func(&in[1]);
			str += " v" + std::to_string(temp->vars_at_id.back()) + '[';
			str.insert(str.end(), arr_size_as_bin.begin(), arr_size_as_bin.end());
			str += "] = " + type + "[]";

			temp->does_array_contain_struct.insert({ temp->vars_at_id.back(), !(in[0] < version->types_code_names.size()) });
			temp->vars_at_id.back()++;
			temp->temp_vars_types.push_back(6);

			return { binary_to_glsl_conversion_exception::ArrayLiteral, {str.begin(), str.end()} };
		}
	});

	//struct ?n
	version->glsl_signatures_alternatives.push_back({ false, 0,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Common;

			temp->context = Temp::Context::StructDeclaration;
			temp->deepness++;
			temp->structs.push_back({});
			temp->vars_at_id.push_back(0);
			
			std::string str = "struct s";
			str += std::to_string(temp->structs.size() - 1) + " {";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//?t ?n ?f
	version->glsl_signatures_alternatives.push_back({ false, 1, 
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Common;
			temp->context = Temp::Context::CustomFunction;
			temp->deepness = 1;
			temp->vars_at_id.push_back(temp->vars_at_id.back());
			temp->function_return_types.push_back(ReturnType(in[0]));
			std::string str = version->types_code_names.at(in[0]) + " f" + std::to_string(temp->FuncCounter);
			temp->FuncCounter++;
			return { binary_to_glsl_conversion_exception::FunctionHeader, {str.begin(), str.end()} };
		}	
	});

	//using layout ?t
	version->glsl_signatures_alternatives.push_back({ false , 1,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Vertex;
			temp->vertex_layout_type = in[0];
			//Basic type like int, float etc
			if (in[0] < version->types_code_names.size())
			{
				temp->vertex_layout_is_struct = false;
				std::string str = "layout (location = 0) in ";
				str += version->types_code_names.at(in[0]) + " i0" + ';';
				temp->vertex_variables = 1;
				temp->vertex_vars_types.push_back(in[0]);
				return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
			}
			//Struct
			//As GLSL doesn't support using structs as input we need to 'unload'
			//So we just do everything as above for each struct member
			else
			{
				temp->vertex_layout_is_struct = true;
				int i = 0;
				std::vector<char> result;
				for (i = 0; i < temp->structs.at(in[0] - version->types_code_names.size()).size(); i++)
				{
					int type = temp->structs.at(in[0] - version->types_code_names.size()).at(i);
					std::string str = "layout (location = " + std::to_string(i) + ") in ";
					str += version->types_code_names.at(type) + " i" + std::to_string(i) + ';';
					temp->vertex_variables++;
					temp->vertex_vars_types.push_back(type);
					result.insert(result.end(), str.begin(), str.end());
				}
				return { binary_to_glsl_conversion_exception::None, result };
			}	
		}
	});

	//using extern ?t ?n
	version->glsl_signatures_alternatives.push_back({true , 1,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Common;
			//To do: uniform structs
			std::string str = "uniform " + 
				((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size()))) 
				+ " v"
				+ std::to_string(temp->vars_at_id.back());
			temp->vars_at_id.at(0)++;
			temp->uniforms_types.push_back(in[0]);

			temp->is_struct.push_back(!(in[0] < version->types_code_names.size()));

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//using extern ?t ?n [ ?i ]
	version->glsl_signatures_alternatives.push_back({ true , 5,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			auto arr_size = decode_int(&in[1]);

			temp->write_target = Temp::WriteTarget::Common;
			//To do: uniform structs
			std::string str = "uniform " +
				((in[0] < version->types_code_names.size()) ?
				version->types_code_names[in[0]] :
				"s" + (std::to_string(in[0] - version->types_code_names.size())))
				+ " v"
				+ std::to_string(temp->vars_at_id.back()) + '[' + arr_size + ']';
			temp->vars_at_id.at(0)++;
			temp->uniforms_types.push_back(in[0]);

			temp->is_struct.push_back(!(in[0] < version->types_code_names.size()));
			temp->does_array_contain_struct.insert({ temp->vars_at_id.at(0) - 1,
				!(in[0] < version->types_code_names.size()) });

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
		});

	//using geometry limit ?i
	version->glsl_signatures_alternatives.push_back({false, 4,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Geometry;

			auto limit = decode_int(&in[0]);
			temp->geometry_shader_vertices_limit = std::stoi(limit);

			std::string str;
			if (temp->geometry_shader_output_primitive_id != -1)
				str = gen_geometry_output_layout_line(temp);

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//using geometry input primitive ?n ?n
	version->glsl_signatures_alternatives.push_back({ true, 1,
	[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Geometry;

			std::string str = "layout(";
			switch (in[0])
			{
			case 0: str += "points"; break;
			case 1: str += "lines"; break;
			case 2: str += "triangles"; break;
			}
			str += ")in";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//using geometry output primitive ?n ?n
	version->glsl_signatures_alternatives.push_back({ false, 1,
	[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->write_target = Temp::WriteTarget::Geometry;

			temp->geometry_shader_output_primitive_id = in[0];

			std::string str;
			if (temp->geometry_shader_vertices_limit != -1)
				str = gen_geometry_output_layout_line(temp);

			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
	});

	//FinishPrimitive
	version->glsl_signatures_alternatives.push_back({ true , 0,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str = "EndPrimitive()";
			return { binary_to_glsl_conversion_exception::None, {str.begin(), str.end()} };
		}
		});

	//send ?t ?n = ?e
	version->glsl_signatures_alternatives.push_back({ true , 0,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			std::string str = 'p' + std::to_string(temp->SentCounter) + '=';
			return { binary_to_glsl_conversion_exception::Send, {str.begin(), str.end()} };
		}
		});

	//send ?t ?n = ?e (overwrite)
	version->glsl_signatures_alternatives.push_back({true , 0,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			return { binary_to_glsl_conversion_exception::SendOverwrite, {} };
		}
	});

	//catch ?t ?n
	version->glsl_signatures_alternatives.push_back({ false , 0,
		[version](std::vector<uint8_t>& in, Temp* temp)
		->std::pair<binary_to_glsl_conversion_exception, std::vector<char>>
		{
			temp->catched_values_ids.push_back(temp->vars_at_id.back());

			for (int i = 1; i < temp->vars_at_id.size(); i++)
				temp->vars_at_id[i]++;

			return { binary_to_glsl_conversion_exception::Catch, {} };
		}
		});

#undef Insert

	return std::unique_ptr<StandardVersion>{ version };
}