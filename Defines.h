#pragma once

// put global defines here
#include <cstdarg>
#include <cstdint>

// shortcuts to avoid typing two characters...
typedef uint64_t uint64;
typedef int64_t sint64;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef uint8_t uint8;
typedef int8_t sint8;
typedef uint8 byte;
typedef sint32 sint;
typedef uint32 uint;

#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4091) // 'keyword' : ignored on left of 'type' when no variable is declared
#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable: 4505) // unreferenced local function has been removed
#pragma warning(disable: 6509) // warning c6509: Return used on precondition
#pragma warning(disable: 4351) //warning C4351: new behavior: elements of array 'x' will be default initialized

//class needs to have dll-interface to used by clients of class. Except that it doesn't. 
//If it did, the linker would complain.
#pragma warning(disable:4251)

#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// breakpoint
#if defined(_M_IX86)
#define UL_INTERRUPT  { __asm { __asm int 3 } }
#elif defined(_M_AMD64)
#define UL_INTERRUPT  { __debugbreak(); }
#endif

// assert that is not removed in release mode
#define UL_RELEASE_ASSERT(expression) { __analysis_assume(expression); if (!(expression)) UL_INTERRUPT; }

// assert
#ifdef DEBUG
#	define UL_ASSERT(expression) \
   {                             \
	   __analysis_assume(expression); \
      if (!(expression))         \
         UL_INTERRUPT;            \
   }
#else
#	define UL_ASSERT(expression) \
   {                             \
	   __analysis_assume(expression); \
   }
#endif

// assumption optimization
#ifdef DEBUG
#	define UL_ASSUME(x) UL_ASSERT(x)
#elif defined(NDEBUG) && defined(_MSC_VER)
#	define UL_ASSUME(x) __assume(x)
#else
#	define UL_ASSUME(x)
#endif

// unreachable code
#	ifdef DEBUG
#		define UL_UNREACHABLE UL_INTERRUPT
#	else
#		define UL_UNREACHABLE __assume(false);
#	endif

#define UL_UNIMPLEMENTED UL_INTERRUPT
#define UL_UNTESTED UL_INTERRUPT

// deprecated code
#define UL_DEPRECATED(x) __declspec(deprecated(x))

// inlined function calls
#if defined(NDEBUG) && defined(_MSC_VER)
#	define UL_INLINE __forceinline
#else
#	define UL_INLINE inline
#endif

// countof for static array
template <typename T, size_t N>
char ( &_ArraySizeHelper( T (&array)[N] ))[N];

#define countof( array ) (sizeof( _ArraySizeHelper( array ) ))

// This can be used as a workaround for when a template type in a class is needed.
// something like MACRO(Class<Param1, Param2>) will make the preprocessor think two
// parameters are supplied ("Class<Param1" and "Param2>").
#define UL_NOOP(...) __VA_ARGS__

template<int stackBufferLength, typename Char> class FormattedString;

template<int stackBufferLength> class FormattedString<stackBufferLength, char>
{

public:

	char stackBuffer[stackBufferLength+1];
	char* heapBuffer;
	const char* value;
	int length;

	FormattedString(const char* format, ...)
	{
		va_list arguments;
		va_start(arguments, format);
		length = vsnprintf(stackBuffer, stackBufferLength, format, arguments);
		if (length >= 0)
		{
			// The formatted string fit on the stack. Use the stack buffer.
			stackBuffer[length] = '\0'; // Fix terminator. Only necessary if length == stackBufferLength.
			heapBuffer = 0;
			value = stackBuffer;
		}
		else
		{
			// The formatted string did not fit on the stack. Allocate a heap buffer.
			length = _vscprintf(format, arguments);
			heapBuffer = new char[length + 1];
			vsprintf(heapBuffer, format, arguments);
			value = heapBuffer;
		}
		va_end(arguments);
	}

	~FormattedString()
	{
		delete[] heapBuffer;
	}
	
	const char* getValue() const { return value; }
	const int getLength() const { return length; }
};

template<int stackBufferLength> class FormattedString<stackBufferLength, wchar_t>
{

public:

	wchar_t stackBuffer[stackBufferLength+1];
	wchar_t* heapBuffer;
	const wchar_t* value;
	int length;

	FormattedString(const wchar_t* format, ...)
	{
		va_list arguments;
		va_start(arguments, format);
		length = _vsnwprintf(stackBuffer, stackBufferLength, format, arguments);
		if (length >= 0)
		{
			// The formatted string fit on the stack. Use the stack buffer.
			stackBuffer[length] = '\0'; // Fix terminator. Only necessary if length == stackBufferLength.
			heapBuffer = 0;
			value = stackBuffer;
		}
		else
		{
			// The formatted string did not fit on the stack. Allocate a heap buffer.
			length = _vscwprintf(format, arguments);
			heapBuffer = new wchar_t[length + 1];
			_vsnwprintf(heapBuffer, length + 1, format, arguments);
			value = heapBuffer;
		}
		va_end(arguments);
	}

	~FormattedString()
	{
		delete[] heapBuffer;
	}
	
	const wchar_t* getValue() const { return value; }
	const int getLength() const { return length; }
};

// Format a string (sprintf style) using a stack buffer of a given size if possible, or a heap buffer otherwise.
#define UL_FORMAT(maxFormattedLength, format, ...) FormattedString<maxFormattedLength, char>(format, __VA_ARGS__).getValue()
#define UL_FORMAT_WIDE(maxFormattedLength, format, ...) FormattedString<maxFormattedLength, wchar_t>(format, __VA_ARGS__).getValue()
