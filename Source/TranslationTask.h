#pragma once
#include "USL_Translator.h"
#include "TempData.h"

class TranslationTask
{
public:
	USL_Translator::TranslationResult result;
	void Start(void* source, int size, Temp* owning_task_temp);
private:
	Temp* temp;
};