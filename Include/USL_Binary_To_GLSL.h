#pragma once

#include "USL_Translator.h"

namespace USL_Translator
{
	class USL_Binary_To_GLSL : public TranslatorBase
	{
	public:
		TranslationResult Translate(Data InData);
		virtual const char* src_type() { return src_type_n; };
		virtual const char* target_type() { return target_type_n; };
	private:
		static const char* src_type_n;
		static const char* target_type_n;
	};
}
