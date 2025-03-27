#pragma once

#ifdef FOURINONEDLL_EXPORTS
#define FOURINONEDLL_API __declspec(dllexport)
#else
#define FOURINONEDLL_API __declspec(dllimport)
#endif

extern "C" FOURINONEDLL_API int SayHello();