#include "LoadMathExpression.h"
#include "ReturnDataType.h"
#include <string>
#include <memory>
#include <functional>

struct Node
{
	enum class Type
	{
		Variable, BasicMathOperator, Literal, Function, VecConstructor, GetOperator, ArrayGet
	} type = Type::Literal;

	ReturnType r_type = ReturnType::Other;

	std::vector<uint8_t> content;
	std::vector<std::unique_ptr<Node>> owned;
	Node* upper = nullptr;
	Node(uint8_t*& iterator, int& next_owned, const StandardVersion* version, const Temp* temp)
	{
		//Operator
		if (*iterator < 32)
		{
			type = *iterator < 4 ? Type::BasicMathOperator : Type::VecConstructor;
			switch (*iterator)
			{
			case 0: content = { '+' }; break;
			case 1: content = { '-' }; break;
			case 2: content = { '*' }; break;
			case 3: content = { '/' }; break;
			case 4: content = { '^' }; break;
			case 5: content = { 'v','e','c','2' }; break;
			case 6: content = { 'v','e','c','3' }; break;
			case 7: content = { 'v','e','c','4' }; break;
			case 8: type = Type::GetOperator; break;
			case 9: type = Type::ArrayGet; break;
			}
			next_owned = *iterator < 5 ? 2 : *iterator - 3;
			if (type == Type::GetOperator)
				next_owned = 0;
			iterator++;
		}
		//Literal
		else if (*iterator >= 64 && *iterator < 96)
		{
			content = version->literal_decoding_functions.at(*iterator - 64).func(iterator + 1);
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
			int func_id = (*iterator) - 96;
			r_type = temp->function_return_types.at(func_id);
			std::string str = 'f' + std::to_string(func_id) + '(';
			next_owned = temp->function_args_count.at(func_id);
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
				if (*iterator - 128 + offset < 0)
				{
					name = 's' + std::to_string(temp->vertex_layout_type - 5) + '(';
					for (int i = 0; i < temp->vertex_variables; i++)
						name += 'i' + std::to_string(i) + (i + 1 != temp->vertex_variables ? ',' : ')');
				}
				else
					name = "v" + std::to_string(*iterator - 128 + offset);
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
			if (*(iterator - 1) >= 128)
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

	tree_to_exp = [&tree_to_exp, &temp](const Node* n) -> std::pair<decltype(Node::content), ReturnType>
	{
		if (n->type == Node::Type::Variable || 
			n->type == Node::Type::Literal)
			return { n->content, n->r_type };

		else if (n->type == Node::Type::Function)
		{
			std::vector<uint8_t> call = n->content;
			int func_id = std::stoi(std::string(n->content.begin() + 1, n->content.end()));
			for (int i = 0; i < n->owned.size(); i++)
			{
				auto arg = tree_to_exp(n->owned[i].get());
				bool add_ending_bracket = false;
				if (arg.second == ReturnType::Float && temp->function_args_types.at(func_id)[i] == ReturnType::Int)
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
			for (int i = 0; i < n->owned.size(); i++)
			{
				auto arg = tree_to_exp(n->owned[i].get());
				result.insert(result.end(), arg.first.begin(), arg.first.end());
				if (i != n->owned.size() - 1)
					result.push_back(',');
			}
			result.push_back(')');
			return { result, ReturnType::Other };
		}
	};

	return tree_to_exp(root.get()).first;
}