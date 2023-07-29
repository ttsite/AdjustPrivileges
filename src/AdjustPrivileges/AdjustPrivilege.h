#pragma once

#include "pch.h"

class AdjustPrivilege
{

};

//Õª×Ô£º\wrk\WindowsResearchKernel-WRK\WRK-v1.2\public\internal\base\inc\zwapi.h
EXTERN_C
NTSTATUS ZwAdjustPrivilegesToken(IN HANDLE TokenHandle,
                                 IN BOOLEAN DisableAllPrivileges,
                                 IN PTOKEN_PRIVILEGES NewState OPTIONAL,
                                 IN ULONG BufferLength OPTIONAL,
                                 OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
                                 OUT PULONG ReturnLength);

NTSTATUS AdjustPrivileges();
NTSTATUS SetTokenInformations();
void ChangeToken();
