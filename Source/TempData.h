#pragma once
#include "ReturnDataType.h"

struct Temp
{
	enum class Context
	{
		GlobalScope, Shader, CustomFunction, StructDeclaration
	};

	enum class ShaderType
	{
		None, VertexShader, PixelShader, GeometryShader
	};

	enum class WriteTarget
	{
		Common, Vertex, Fragment, Geometry
	};

	enum class FileType
	{
		Shader, Library, Metashader
	};

	Context context = Context::GlobalScope;
	ShaderType shader_type = ShaderType::None;
	WriteTarget write_target = WriteTarget::Common;
	FileType file_type = FileType::Shader;

	/*
		Each value represents variables count at given deepness
		so vars_at_id[2] is count of vars at deepness 2

		*Each variable name = 'v' + string(vars_at_id.back())
		*Always when new variable is created increment last value of vector as number of variables increases
		*When deepness increases push value from previous deepness (vars_at_id.push_back(vars_at_id.back()))
		*When deepness decreases remove last element of the vector as this block doesn't exist anymore
		
		By following this algorithm we ensure that no name will occur twice in the same block of code
	*/
	//Global scope doesn't contains any vars
	std::vector<int> vars_at_id = { 0 };

	/*
		Number of vars used to store input vertex data. 
		It's value change when parsing "using layout" instruction
		It is read when parsing "Vertex Shader". vars_at_id.at(1) is given value of vertex_variables
		because othervise if vars_at_id.at(1) equals 0 variables names will interfere
	*/
	int vertex_variables = 0;
	std::vector<int> vertex_vars_types;

	std::vector<int> uniforms_types;

	std::vector<int> temp_vars_types;

	std::vector<int> sent_vars_types;

	//Array variable id, contains structs
	std::map<int, bool> does_array_contain_struct;

	int deepness = 0;

	/*
		Contains information whether variable is struct or not
		Used to decide if use .x, .y, .z or .m0, .m1 ... when handling get operator
	*/
	std::vector<bool> is_struct = {};

	/*
		Contains structs layout
		structs.at(a)		<- return layout of struct of id equal to a
		structs.at(a).at(b) <- return layout of b member of struct a
	*/
	std::vector<std::vector<int>> structs;

	int SentCounter = 0;

	int FuncCounter = 0;

	std::vector<int> catched_values_ids;

	//id is function id, value is ammount of args
	std::vector<int> function_args_count;
	std::vector<ReturnType> function_return_types;
	std::vector<std::vector<ReturnType>> function_args_types;

	bool vertex_layout_is_struct = false;

	int vertex_layout_type = -1;

	int geometry_shader_vertices_limit = -1;
	int geometry_shader_output_primitive_id = -1;

	std::vector<int> maunaly_managed_global_access_buffor;
};