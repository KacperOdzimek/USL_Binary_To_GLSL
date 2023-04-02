#pragma once

struct Temp
{
	enum class Context
	{
		GlobalScope, Shader, CustomFunction, StructDeclaration
	};

	enum class ShaderType
	{
		VertexShader, PixelShader
	};

	Context context = Context::GlobalScope;
	ShaderType shader_type = ShaderType::VertexShader;

	int deepness = 0;
};