#pragma once
#include "USL_Translator.h"

class TranslateTask
{
public:
	USL_Translator::TranslationResult result;
	void Start(void* source, int size);
};