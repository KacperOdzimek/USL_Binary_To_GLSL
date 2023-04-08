#include "LoadMathExpression.h"
#include <string>
#include <memory>
#include <functional>

struct Node
{
	enum class Type
	{
		Variable, BasicMathOperator, Literal, Function, VecConstructor, GetOperator
	} type = Type::Literal;
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
			iterator += version->literal_decoding_functions.at(*iterator - 64).literal_size + 1;
			next_owned = 0;
		}
		//Function
		else if (*iterator >= 96 && *iterator < 128)
		{
			//TO DO
		}
		//Variable
		else if (*iterator >= 128)
		{
			std::string name;
			type = Type::Variable;
			for (int i = 0; i < temp->catched_values_ids.size(); i++)
				if (*iterator - 128 == temp->catched_values_ids[i])
					name = "p" + std::to_string(i);
			if (name.size() == 0)
			{
				int offset = 0;
				if (temp->shader_type == Temp::ShaderType::VertexShader)
					offset = -1;
				name = "v" + std::to_string(*iterator - 128 + offset);
			}
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

	/*auto get_var_id = [temp](int raw) -> int
	{
		if (raw == 128 && temp->vertex_variables != 1 && temp->shader_type == Temp::ShaderType::VertexShader)

		else
		{
			return raw;
	};*/

	get_node = [&iterator, &get_node, version, temp]()
	{
		int own_requested = 0;
		auto node = std::make_unique<Node>(iterator, own_requested, version, temp);

		if (node->type == Node::Type::GetOperator)
		{
			node = get_node();

			int id = *iterator;
			if (*(iterator - 1) == 128 && 
				temp->vertex_layout_is_struct && temp->shader_type == Temp::ShaderType::VertexShader)
			{
				node->content.clear();
				auto var_id = std::string("i") + std::to_string(id);
				node->content.insert(node->content.end(), var_id.begin(), var_id.end());
			}
			/*else if (temp->is_struct[*(iterator - 1) - 128])
			{
				std::string str = ".m";
				str += std::to_string(id);
				node->content.insert(node->content.end(), str.begin(), str.end());
			}*/
			else
			{
				node->content.push_back('.');
				switch (id)
				{
				case 0: node->content.push_back('x'); break;
				case 1: node->content.push_back('y'); break;
				case 2: node->content.push_back('z'); break;
				case 3: node->content.push_back('w'); break;
				}
			}

			++iterator;
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

	std::function<std::vector<uint8_t>(const Node* n)> tree_to_exp;

	tree_to_exp = [&tree_to_exp](const Node* n)
	{
		if (n->type == Node::Type::Variable || 
			n->type == Node::Type::Literal)
			return n->content;
		else if (n->type == Node::Type::BasicMathOperator)
		{
			auto lhs = tree_to_exp(n->owned[0].get());
			auto rhs = tree_to_exp(n->owned[1].get());
			lhs.insert(lhs.end(), n->content.begin(), n->content.end());
			lhs.insert(lhs.end(), rhs.begin(), rhs.end());
			return lhs;
		}
		else if (n->type == Node::Type::VecConstructor)
		{
			auto result = n->content;
			result.push_back('(');
			for (int i = 0; i < n->owned.size(); i++)
			{
				auto arg = tree_to_exp(n->owned[i].get());
				result.insert(result.end(), arg.begin(), arg.end());
				if (i != n->owned.size() - 1)
					result.push_back(',');
			}
			result.push_back(')');
			return result;
		}
	};

	return tree_to_exp(root.get());
}