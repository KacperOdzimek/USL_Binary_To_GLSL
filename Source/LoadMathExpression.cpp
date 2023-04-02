#include "LoadMathExpression.h"
#include <string>
#include <memory>
#include <functional>

struct Node
{
	enum class Type
	{
		Variable, Operator, Literal, Function
	} type = Type::Literal;
	std::vector<uint8_t> content;
	std::vector<std::unique_ptr<Node>> owned;
	Node* upper = nullptr;
	Node(int& i, std::vector<uint8_t>& src, int& next_owned)
	{
		//Operator
		if (src[i] < 32)
		{
			type = Type::Operator;
			switch (src[i])
			{
			case 0: content = { '+' }; break;
			case 1: content = { '-' }; break;
			case 2: content = { '*' }; break;
			case 3: content = { '/' }; break;
			}
			next_owned = 2;
		}
		//Literal
		else if (src[i] >= 64)
		{
			i += 4 * 4;
			content = { 'v' };
			next_owned = 0;
		}
		//Function
		if (src[i] >= 96)
		{
			//TO DO
		}
		//Variable
		else if (src[i] >= 128)
		{
			type = Type::Variable;
			std::string name = "v" + std::to_string(src[i] - 128);
			content = std::vector<uint8_t>(name.begin(), name.end());
			next_owned = 0;
		}
	}
};

std::vector<uint8_t> LoadMathExpression(uint8_t*& iterator)
{
	int exp_size = *iterator;

	std::vector<uint8_t> binary;

	for (int i = 0; i < exp_size; i++)
	{
		++iterator;
		binary.push_back(*iterator);
	}
	//Make it pointing to new signature
	++iterator;

	int i = 0;

	std::function<std::unique_ptr<Node>()> get_node;

	get_node = [&binary, &i, &get_node]()
	{
		int own_requested = 0;
		auto node = std::make_unique<Node>(i, binary, own_requested);
		for (int i = 0; i < own_requested; i++)
		{
			auto new_node = get_node();
			new_node->upper = node.get();
			node->owned.push_back(std::move(new_node));
		}
			
		return node;
	};

	auto abc = get_node();

	return binary;
}