#ifndef GENERIC_CPPASS_FFI_H
#define GENERIC_CPPASS_FFI_H

// Helper macro to combine tokens (e.g., to create function names like 'hepf_PathBasedInterProcFanOutPass_new')
#define PASTE_NAME_HELPER(a, b) a ## b
#define PASTE_NAME(a, b) PASTE_NAME_HELPER(a, b)

// --- C Interface Generation Macro ---
// Use this in your C/C++ header (.h) file inside the extern "C" block.
// Arguments: Namespace (e.g., 'hepf'), C++ Class Name (e.g., 'PathBasedInterProcFanOutPass').
#define DECLARE_CPPASS_FFI(NS, CPP_CLASS_NAME) \
    typedef void PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _t))); \
    \
    PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _t)))* \
    PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _new)))(); \
    \
    void \
    PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _delete)))( \
        PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _t)))* self \
    );

// --- C++ Implementation Macro ---
// Use this in your C++ source (.cpp) file.
#define IMPLEMENT_CPPASS_FFI(NS, CPP_CLASS_NAME) \
    namespace NS { \
        PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _t)))* \
        PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _new)))() \
        { \
            std::cout << "C++: Creating new " << #NS << "::" << #CPP_CLASS_NAME << " object." << std::endl; \
            return new ::NS::CPP_CLASS_NAME(); \
        } \
        \
        void \
        PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _delete)))( \
            PASTE_NAME(NS, PASTE_NAME(_, PASTE_NAME(CPP_CLASS_NAME, _t)))* self \
        ) \
        { \
            if (self) { \
                std::cout << "C++: Deleting " << #NS << "::" << #CPP_CLASS_NAME << " object." << std::endl; \
                delete static_cast<::NS::CPP_CLASS_NAME*>(self); \
            } \
        } \
    }

#endif // GENERIC_CPPASS_FFI_H
