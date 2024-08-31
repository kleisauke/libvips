#ifndef VIPS_STACKTRACE_H
#define VIPS_STACKTRACE_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <stdio.h>

#ifdef G_OS_UNIX
#include <execinfo.h>
#endif

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#include <dbghelp.h>
#endif

#ifndef G_ALWAYS_INLINE
#define G_ALWAYS_INLINE /* empty */
#endif

G_ALWAYS_INLINE static inline void
print_stacktrace()
{
#ifdef G_OS_UNIX
	void *stack[42];
	int frames = backtrace(stack, 42);
	char **strs = backtrace_symbols(stack, frames);

	for (int i = 0; i < frames; ++i)
		printf("%i: %s\n", frames - i - 1, strs[i]);
	free(strs);
#elif defined(G_OS_WIN32)
	// From: https://stackoverflow.com/a/5699483
	void *stack[42];
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	USHORT frames = CaptureStackBackTrace(0, 42, stack, NULL);
	PSYMBOL_INFO symbol =
		(PSYMBOL_INFO) calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(TCHAR), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (USHORT i = 0; i < frames; i++) {
		SymFromAddr(process, (DWORD64) (stack[i]), 0, symbol);

		printf("%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name,
			symbol->Address);
	}
	free(symbol);
#endif
}

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_STACKTRACE_H*/
