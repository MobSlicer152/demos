#include "pch.h"
#include "demolib.h"

extern "C"
{
	UINT_PTR __security_cookie;
	int _fltused = 0; // forget what the exact value of this means

	void _RTC_InitBase()
	{
	}

	void _RTC_Shutdown()
	{
	}

	void _RTC_CheckStackVars()
	{
	}

	void __GSHandlerCheck()
	{
	}

	// note: must be asm if actually given a body, because it cant have a prologue/epilogue
	void __security_check_cookie(UINT_PTR other)
	{
	}

	void __report_rangecheckfailure()
	{
	}
}
