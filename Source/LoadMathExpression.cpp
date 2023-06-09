#include "LoadMathExpression.h"
#include "ReturnDataType.h"
#include <string>
#include <memory>
#include <functional>

struct Node
{
	enum class Type
	{
		Variable, BasicMathOperator, Literal, Function, VecConstructor, GetOperator, ArrayGet, MatrixGet, StructConstructor
	} type = Type::Literal;

	ReturnType r_type = ReturnType::Other;

	std::vector<uint8_t> content;
	std::vector<std::unique_ptr<Node>> owned;
	Node* upper = nullptr;
	Node(uint8_t*& iterator, int& next_owned, const StandardVersion* version, Temp* temp)
	{
		//Struct Constructor
		if (*iterator == 32)
		{
			type = Type::StructConstructor;
			uint8_t id = *(++iterator);
			std::string str = "s" + std::to_string(id) + '(';
			content.insert(content.end(), str.begin(), str.end());
			next_owned = temp->structs.at(id).size();
			iterator += 1;
		}
		//Operator
		else if (*iterator < 32)
		{
			type = *iterator < 4 ? Type::BasicMathOperator : Type::VecConstructor;
			switch (*iterator)
			{
			case 0: content = { '+' }; break;
			case 1: content = { '-' }; break;
			case 2: content = { '*' }; break;
			case 3: content = { '/' }; break;
			case 4: content = { 'p','o','w'}; break;
			case 5: content = { 'v','e','c','2' }; break;
			case 6: content = { 'v','e','c','3' }; break;
			case 7: content = { 'v','e','c','4' }; break;
			case 8: type = Type::GetOperator; break;
			case 9: type = Type::ArrayGet; break;
			case 10: type = Type::MatrixGet; break;
			}
			next_owned = *iterator < 5 ? 2 : *iterator - 3;
			if (type == Type::GetOperator)
				next_owned = 0;
			if (type == Type::MatrixGet)
				next_owned = 0;
			iterator++;
		}
		//Literal
		else if (*iterator >= 64 && *iterator < 96)
		{
			content = version->literal_decoding_functions.at(*iterator - 64).func(iterator + 1);

			if (*iterator - 64 >= version->types_code_names.size())
				r_type = ReturnType::Struct;
			else
				switch (*iterator - 64)
				{
					case 0:  r_type = ReturnType::Int;   break;
					case 1:  r_type = ReturnType::Float; break;
					default: r_type = ReturnType::Other; break;
				}
			iterator += version->literal_decoding_functions.at(*iterator - 64).literal_size + 1;
			next_owned = 0;
		}
		//Function
		else if (*iterator >= 96 && *iterator < 128)
		{
			type = Type::Function;
			uint8_t func_id_1 = (*iterator) - 96;
			++iterator;
			uint8_t func_id_2 = *(iterator);
			uint16_t func_id = ((uint16_t)func_id_1 << 8) | func_id_2;
			//User definied
			std::string str;
			if (func_id >= version->built_in_functions.size())
			{
				func_id -= version->built_in_functions.size();
				r_type = temp->function_return_types.at(func_id);
				str = 'f' + std::to_string(func_id) + '(';
				next_owned = temp->function_args_count.at(func_id);
			}
			//Built-In
			else
			{
				r_type = ReturnType(version->built_in_functions.at(func_id).return_id);
				str = version->built_in_functions.at(func_id).glsl_name + '(';
				next_owned = version->built_in_functions.at(func_id).args.size();

				temp->maunaly_managed_global_access_buffor.push_back(func_id);
			}
			iterator++;
			content = { str.begin(), str.end() };
		}
		//Variable
		else if (*iterator >= 128)
		{
			std::string name;
			type = Type::Variable;
			for (int i = 0; i < temp->catched_values_ids.size(); i++)
				if (*iterator - 128 == temp->catched_values_ids[i])
				{
					name = "p" + std::to_string(i);
					r_type = ReturnType(temp->sent_vars_types.at(i));
				}
			if (name.size() == 0)
			{
				int offset = 0;

				if (temp->shader_type == Temp::ShaderType::GeometryShader 
					&& temp->context == Temp::Context::Shader
					&& *iterator - 128 == 0)
				{
					name = "gl_in";
					goto var_out;
				}

				if (temp->shader_type == Temp::ShaderType::VertexShader && temp->context == Temp::Context::Shader)
					offset = -1;
				if (*iterator - 128 == 0 && temp->shader_type == Temp::ShaderType::VertexShader)
					r_type = ReturnType(temp->vertex_vars_types.at(*iterator - 128));
				else
					r_type = ReturnType(temp->temp_vars_types.at(*iterator - 128 + offset));
				//Vertex layout structure reconstruction from it's elements
				if (*iterator - 128 + offset < 0)
				{
					name = 's' + std::to_string(temp->vertex_layout_type - version->types_code_names.size()) + '(';
					for (int i = 0; i < temp->vertex_variables; i++)
						name += 'i' + std::to_string(i) + (i + 1 != temp->vertex_variables ? ',' : ')');
				}
				else
					name = "v" + std::to_string(*iterator - 128 + offset + temp->variable_id_shift_value);
			}
			var_out:
			content = std::vector<uint8_t>(name.begin(), name.end());
			next_owned = 0;
			++iterator;
		}
	}
};

std::vector<uint8_t> LoadMathExpression(uint8_t*& iterator, const StandardVersion* version, Temp* temp)
{
	int exp_size = *iterator;

	//pass the expression size
	++iterator;

	std::function<std::unique_ptr<Node>()> get_node;

	get_node = [&iterator, &get_node, version, temp]()
	{
		int own_requested = 0;
		auto node = std::make_unique<Node>(iterator, own_requested, version, temp);

		if (node->type == Node::Type::GetOperator)
		{
			node = get_node();

			int id = *iterator;
			if (node->r_type == ReturnType::Struct)
			{
				std::string str = ".m";
				str += std::to_string(id);
				node->content.insert(node->content.end(), str.begin(), str.end());
			}
			else if (*(iterator - 1) >= 128)
			{
				//Vertex built in var in Vertex Shader
				if (*(iterator - 1) == 128 &&
					temp->vertex_layout_is_struct && temp->shader_type == Temp::ShaderType::VertexShader)
				{
					node->content.clear();
					auto var_id = std::string("i") + std::to_string(id);
					node->content.insert(node->content.end(), var_id.begin(), var_id.end());
					node->r_type = ReturnType::Float;
				}
				//Structs
				else if
					(
						(temp->context == Temp::Context::CustomFunction &&
							temp->is_struct.size() != temp->vertex_variables &&
							temp->is_struct[*(iterator - 1) + temp->vertex_variables - 128])
						||
						temp->is_struct[*(iterator - 1) - 128]
					)
				{
					std::string str = ".m";
					str += std::to_string(id);
					node->content.insert(node->content.end(), str.begin(), str.end());
				}
				else
					goto els;
			}
			else
			{
				els:
				node->content.push_back('.');
				switch (id)
				{
				case 0: node->content.push_back('x'); break;
				case 1: node->content.push_back('y'); break;
				case 2: node->content.push_back('z'); break;
				case 3: node->content.push_back('w'); break;
				}
				node->r_type = ReturnType::Float;
			}

			++iterator;
		}
		else if (node->type == Node::Type::ArrayGet)
		{
			//Array var
			node = get_node();

			std::string var_id;
			var_id.insert(var_id.begin(), node->content.begin() + 1, node->content.end());

			node->content.push_back('[');
			auto id_node = get_node();
			std::string id;
			if (id_node->r_type == ReturnType::Float)
			{
				std::string id_str = "int(";
				id_str.insert(id_str.end(), id_node->content.begin(), id_node->content.end());
				id_str.push_back(')');
				node->content.insert(node->content.end(), id_str.begin(), id_str.end());
			}
			else
				node->content.insert(node->content.end(), id_node->content.begin(), id_node->content.end());
			
			node->content.push_back(']');

			if (temp->shader_type == Temp::ShaderType::GeometryShader
				&& temp->context == Temp::Context::Shader)
			{
				std::string interface_block_name = "gl_in[";
				bool result = true;
				for (int i = 0; i < interface_block_name.size(); i++)
				{
					if (node->content.at(i) != interface_block_name.at(i))
					{
						result = false;
						break;
					}
				}

				if (result)
				{
					std::string extension = ".gl_Position";
					node->content.insert(node->content.end(), extension.begin(), extension.end());
				}
			}
			
			std::string arr_id_str;
			for (int i = 1; i < node->content.size(); i++)
				if (node->content.at(i) == '[')
					break;
				else
					arr_id_str += node->content.at(i);

			int arr_id = std::stoi(arr_id_str);

			if (arr_id >= temp->does_array_contain_struct.size())
				node->r_type = ReturnType::Other;
			else
				node->r_type = (temp->does_array_contain_struct.at(arr_id))
					? ReturnType::Struct : ReturnType::Other;

			own_requested = 0;
		}
		else if (node->type == Node::Type::MatrixGet)
		{
			//Array var
			node = get_node();

			std::string var_id;
			var_id.insert(var_id.begin(), node->content.begin() + 1, node->content.end());

			std::unique_ptr<Node> id_nodes[2] = { get_node(), get_node() };
			for (int i = 0; i < 2; i++)
			{
				node->content.push_back('[');
				std::string id;
				if (id_nodes[i]->r_type == ReturnType::Float)
				{
					std::string id_str = "int(";
					id_str.insert(id_str.end(), id_nodes[i]->content.begin(), id_nodes[i]->content.end());
					id_str.push_back(')');
					node->content.insert(node->content.end(), id_str.begin(), id_str.end());
				}
				else
					node->content.insert(node->content.end(), id_nodes[i]->content.begin(), id_nodes[i]->content.end());

				node->content.push_back(']');
			}

			own_requested = 0;
		}

		for (int j = 0; j < own_requested; j++)
		{
			auto new_node = get_node();
			new_node->upper = node.get();
			node->owned.push_back(std::move(new_node));
		}
			
		return node;
	};

	auto root = get_node();

	std::function<std::pair<decltype(Node::content), ReturnType>(const Node* n)> tree_to_exp;

	tree_to_exp = [&tree_to_exp, &temp, version](const Node* n) -> std::pair<decltype(Node::content), ReturnType>
	{
		if (n->type == Node::Type::Variable || 
			n->type == Node::Type::Literal)
			return { n->content, n->r_type };

		else if (n->type == Node::Type::Function)
		{
			std::vector<uint8_t> call = n->content;

			std::vector<ReturnType> args_types;

			//If function name ends with a digit then it is user declared one otherwise built-in
			if (n->content.at(n->content.size() - 2) >= '0' && n->content.at(n->content.size() - 2) <= '9')
				args_types =
				temp->function_args_types.at(std::stoi(std::string(n->content.begin() + 1, n->content.end())));
			else
			{
				args_types = version->built_in_functions.at(temp->maunaly_managed_global_access_buffor.at(0)).args;
				temp->maunaly_managed_global_access_buffor.clear();
			}

			for (int i = 0; i < n->owned.size(); i++)
			{
				auto arg = tree_to_exp(n->owned[i].get());
				bool add_ending_bracket = false;
				if (arg.second == ReturnType::Float && args_types[i] == ReturnType::Int)
				{
					std::string conversion = "int(";
					call.insert(call.end(), conversion.begin(), conversion.end());
					add_ending_bracket = true;
				}

				call.insert(call.end(), arg.first.begin(), arg.first.end());
				if (add_ending_bracket) call.push_back(')');
				if (i + 1 != n->owned.size())
					call.push_back(',');
			}
			call.push_back(')');
			return { call, n->r_type };
		}
		else if (n->type == Node::Type::StructConstructor)
		{
			std::vector<uint8_t> str = n->content;

			for (int i = 0; i < n->owned.size(); i++)
			{
				str.insert(str.end(), n->owned.at(i)->content.begin(), n->owned.at(i)->content.end());
				str.push_back(i == n->owned.size() - 1 ? ')' : ',');
			}

			return { str, ReturnType::Struct };
		}
		else if (n->type == Node::Type::BasicMathOperator)
		{
			auto lhs = tree_to_exp(n->owned[0].get());
			auto rhs = tree_to_exp(n->owned[1].get());
			lhs.first.insert(lhs.first.end(), n->content.begin(), n->content.end());
			lhs.first.insert(lhs.first.end(), rhs.first.begin(), rhs.first.end());
			return { lhs.first, (lhs.second > rhs.second) ? lhs.second : rhs.second };
		}
		else if (n->type == Node::Type::VecConstructor)
		{
			auto result = n->content;
			result.push_back('(');
			std::vector<std::vector<uint8_t>> args;
			for (int i = 0; i < n->owned.size(); i++)
				args.push_back(tree_to_exp(n->owned[i].get()).first);

			//Swizzling optimization
			for (int offset = 0; offset < args.size(); offset++)
			{
				result.insert(result.end(), args[offset].begin(), args[offset].end());
				for (int swizzle = offset + 1; swizzle < args.size(); swizzle++)
				{
					if (args[swizzle].size() < 3 || args[swizzle].at(args.size() - 2) != '.')
					{
						offset = swizzle - 1;
						break;
					}
					else
					{
						std::string first_arg_raw = { args[offset].begin(), args[offset].end() };
						std::string vector1 = first_arg_raw.substr(0, first_arg_raw.size() - 2);

						std::string sec_arg_raw = { args[swizzle].begin(), args[swizzle].end() };
						std::string vector2 = sec_arg_raw.substr(0, sec_arg_raw.size() - 2);

						if (vector1 == vector2)
						{
							result.push_back(sec_arg_raw.back());
						}
						else
						{
							offset = swizzle - 1;
							break;
						}
					}
				}
				if (offset != n->owned.size() - 1)
					result.push_back(',');
			}

			result.push_back(')');
			return { result, ReturnType::Other };
		}
	};

	return tree_to_exp(root.get()).first;
}