#include "General.h"
#include "CriticalSectionClass.h"

CriticalSectionClass::CriticalSectionClass()
{
	handle = new CRITICAL_SECTION;
	InitializeCriticalSection((CRITICAL_SECTION*)handle);
	locked = 0;
}



CriticalSectionClass::~CriticalSectionClass()
{
	DeleteCriticalSection((CRITICAL_SECTION*)handle);
	delete handle;
	handle = 0;
}



#pragma warning(suppress: 26135) //warning C26135: Missing annotation
void CriticalSectionClass::Enter()
{
	EnterCriticalSection((CRITICAL_SECTION*)handle);
	locked++;
}



#pragma warning(suppress: 26135) //warning C26135: Missing annotation
void CriticalSectionClass::Exit()
{
	locked--;
	LeaveCriticalSection((CRITICAL_SECTION*)handle);
}



CriticalSectionClass::LockClass::LockClass(CriticalSectionClass &section) : CriticalSection(section)
{
	CriticalSection.Enter();
}



CriticalSectionClass::LockClass::~LockClass()
{
	CriticalSection.Exit();
}