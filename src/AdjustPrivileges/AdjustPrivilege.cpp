#include "AdjustPrivilege.h"


NTSTATUS AdjustPrivilege(ULONG Privilege, BOOLEAN Enable)
/*
功能：给当前进程（上下文所在的进程）提权。

用法示例：AdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE);

摘自：http://www.osronline.com/article.cfm?article=23
*/
{
    NTSTATUS Status;
    TOKEN_PRIVILEGES privSet;
    HANDLE tokenHandle;

    // Open current process token
    Status = ZwOpenProcessTokenEx(NtCurrentProcess(), TOKEN_ALL_ACCESS, OBJ_KERNEL_HANDLE, &tokenHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("NtOpenProcessToken failed, Status 0x%x\n", Status);
        return Status;
    }

    // Set up the information about the privilege we are adjusting
    privSet.PrivilegeCount = 1;
    privSet.Privileges[0].Luid = RtlConvertUlongToLuid(Privilege);
    if (Enable) {
        privSet.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    } else {
        privSet.Privileges[0].Attributes = 0;
    }

    Status = ZwAdjustPrivilegesToken(tokenHandle,
                                     FALSE, // don't disable all privileges
                                     &privSet,
                                     sizeof(privSet),
                                     NULL, // old privileges - don't care
                                     NULL); // returned length
    if (!NT_SUCCESS(Status)) {
        DbgPrint("ZwAdjustPrivilegesToken failed, Status 0x%x\n", Status);
    }

    (void)ZwClose(tokenHandle);// Close the process token handle

    return Status;
}


NTSTATUS AdjustPrivileges()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Status = AdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE);
    if (!NT_SUCCESS(Status)) {
        Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", Status);
    }

    return Status;
}


NTSTATUS SetTokenInformation(_In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
                             _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
                             _In_ ULONG TokenInformationLength
)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CLIENT_ID   ClientId = {0};
    OBJECT_ATTRIBUTES ob;
    HANDLE  ProcessHandle;
    HANDLE TokenHandle = 0;

    ClientId.UniqueProcess = PsGetCurrentProcessId();
    InitializeObjectAttributes(&ob, 0, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
    Status = ZwOpenProcess(&ProcessHandle, GENERIC_ALL, &ob, &ClientId);
    ASSERT(NT_SUCCESS(Status));

    Status = ZwOpenProcessTokenEx(ProcessHandle, TOKEN_ALL_ACCESS, OBJ_KERNEL_HANDLE, &TokenHandle);
    ASSERT(NT_SUCCESS(Status));

    Status = ZwSetInformationToken(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);

    Status = ZwClose(TokenHandle);
    Status = ZwClose(ProcessHandle);
    return Status;
}


NTSTATUS SetTokenDefaultDacl()
/*

https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_token_default_dacl
*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;


    return Status;
}


NTSTATUS SetTokenOwner()
/*

https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_token_owner
*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    TOKEN_OWNER OwnerToken{};
    BYTE Sid[MAX_PATH]{};
    ULONG SidSize = 0;

    Status = SecLookupWellKnownSid(WinLocalSystemSid, Sid, sizeof(Sid), &SidSize);
    if (!NT_SUCCESS(Status)) {
        Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", Status);
    }

    OwnerToken.Owner = (PSID)&Sid;
    Status = SetTokenInformation(TokenOwner, &OwnerToken, sizeof(OwnerToken));
    if (!NT_SUCCESS(Status)) {
        Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", Status);
    }

    return Status;
}


NTSTATUS SetTokenPrimaryGroup()
/*

https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_token_primary_group
*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    TOKEN_PRIMARY_GROUP  OwnerToken{};
    BYTE Sid[MAX_PATH]{};
    ULONG SidSize = 0;

    Status = SecLookupWellKnownSid(WinLocalSystemSid, Sid, sizeof(Sid), &SidSize);
    if (!NT_SUCCESS(Status)) {
        Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", Status);
    }

    OwnerToken.PrimaryGroup = (PSID)&Sid;
    Status = SetTokenInformation(TokenPrimaryGroup, &OwnerToken, sizeof(OwnerToken));
    if (!NT_SUCCESS(Status)) {
        Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", Status);
    }    

    return Status;
}


NTSTATUS SetTokenInformations()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Status = SetTokenDefaultDacl();
    Status = SetTokenOwner();
    Status = SetTokenPrimaryGroup();

    return Status;
}


void ChangeToken()
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CLIENT_ID   ClientId = {0};
    OBJECT_ATTRIBUTES ob;
    HANDLE  ProcessHandle;
    HANDLE TokenHandle = 0;

    ClientId.UniqueProcess = PsGetCurrentProcessId();
    InitializeObjectAttributes(&ob, 0, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
    Status = ZwOpenProcess(&ProcessHandle, GENERIC_ALL, &ob, &ClientId);
    ASSERT(NT_SUCCESS(Status));

    Status = ZwOpenProcessTokenEx(ProcessHandle, TOKEN_ALL_ACCESS, OBJ_KERNEL_HANDLE, &TokenHandle);
    ASSERT(NT_SUCCESS(Status));

    PVOID ProcessObject = nullptr;
    Status = ObReferenceObjectByHandle(ProcessHandle, GENERIC_ALL, *PsProcessType, KernelMode, &ProcessObject, 0);
    ASSERT(NT_SUCCESS(Status));

    PVOID * TokenObject = (PVOID *)((PBYTE)ProcessObject + g_TokenOffset);

    InterlockedExchangePointer(TokenObject, g_SystemTokenObject);//核心重点。不知会触发PG不？

    ObDereferenceObject(ProcessObject);
    Status = ZwClose(TokenHandle);
    Status = ZwClose(ProcessHandle);
}
