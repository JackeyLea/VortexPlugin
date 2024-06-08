#pragma once

#if defined(_MSC_VER)
    #ifdef VORTEXINTEGRATION_EXPORT
        #define VORTEXINTEGRATION_SYMBOL __declspec(dllexport)
    #else
        #define VORTEXINTEGRATION_SYMBOL __declspec(dllimport)
    #endif
#else
    #define VORTEXINTEGRATION_SYMBOL
#endif

