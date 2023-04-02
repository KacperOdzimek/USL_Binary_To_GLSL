#include "../Include/USL_Binary_To_GLSL.h"
#include "TranslationTask.h"

#include <vector>
#include <memory>

const char* USL_Translator::USL_Binary_To_GLSL::src_type_n =    "USL_Binary";
const char* USL_Translator::USL_Binary_To_GLSL::target_type_n = "GLSL";

USL_Translator::TranslationResult USL_Translator::USL_Binary_To_GLSL::Translate(Data InData)
{
	std::unique_ptr<TranslationTask> task = std::make_unique<TranslationTask>();
	task->Start(InData.position, InData.size);
	return task->result;
}