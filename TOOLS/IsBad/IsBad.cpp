#include "IsBad.h"
#include "windows.h"

#pragma warning(disable:4800)

bool F4IsBadReadPtr(const void* lp, unsigned int ucb)
{
	return IsBadReadPtr(lp, ucb);
}

bool F4IsBadCodePtr(void* lpfn)
{
	return IsBadCodePtr((FARPROC) lpfn);
}

bool F4IsBadWritePtr(void* lp, unsigned int ucb)
{
	return IsBadWritePtr(lp, ucb);
}