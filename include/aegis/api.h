#pragma once

#if defined(_WIN32)

#if defined(AEGIS_BUILD_DLL)
#define AEGIS_API __declspec(dllexport)
#else
#define AEGIS_API __declspec(dllimport)
#endif

#else

#define AEGIS_API

#endif