#include "DriverEntry.h"
#include "AdjustPrivilege.h"

#define CSAMP_DEVICE_NAME_U     L"\\Device\\AdjustPrivilege"
#define CSAMP_DOS_DEVICE_NAME_U L"\\DosDevices\\AdjustPrivilege"

// {5D006E1A-2631-466c-B8A0-32FD498E4424}  - generated using guidgen.exe
DEFINE_GUID(GUID_DEVCLASS_CANCEL_SAMPLE,
            0x5d006e1a, 0x2631, 0x466c, 0xb8, 0xa0, 0x32, 0xfd, 0x49, 0x8e, 0x44, 0x24);


_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID Unload(_In_ struct _DRIVER_OBJECT * DriverObject)
{
    PDEVICE_OBJECT      deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING      uniWin32NameString;

    PAGED_CODE();

    RtlInitUnicodeString(&uniWin32NameString, CSAMP_DOS_DEVICE_NAME_U);

    IoDeleteSymbolicLink(&uniWin32NameString);

    IoDeleteDevice(deviceObject);
}


NTSTATUS CreateDeviceSecure(_In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      unicodeDeviceName;
    UNICODE_STRING      unicodeDosDeviceName;
    PDEVICE_OBJECT      deviceObject;
    UNICODE_STRING      sddlString;

    (void)RtlInitUnicodeString(&unicodeDeviceName, CSAMP_DEVICE_NAME_U);

    /*
    字符串含义：
    1.SY：System SDDL_ACCESS_ALLOWED GENERIC_ALL
    2.BA：Administrators（The built-in Administrators group on the machine.） 拥有所有的权限。
    3.BU：Built-in User Group（Group covering all local user accounts, and users on the domain.）这个是关键。
    4.BG：Built-in Guest Group（Group covering users logging in using the local or domain guest account.）
    5.AU：Authenticated Users
    6.AN：Anonymous Logged-on User
    7.IU：Interactive Users

    参考：
    https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/sddl-for-device-objects
    https://learn.microsoft.com/en-us/windows/win32/secauthz/ace-strings
    */
    (void)RtlInitUnicodeString(&sddlString, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BU)(A;;GA;;;BG)(A;;GA;;;AU)(A;;GA;;;AN)(A;;GA;;;IU)");
    /*
    经测试：普通的users组的用户可以打开设备对象。
    */
    
    status = IoCreateDeviceSecure(
        DriverObject,
        0,
        &unicodeDeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        (BOOLEAN)FALSE,
        &sddlString,
        (LPCGUID)&GUID_DEVCLASS_CANCEL_SAMPLE,
        &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    (void)RtlInitUnicodeString(&unicodeDosDeviceName, CSAMP_DOS_DEVICE_NAME_U);
    status = IoCreateSymbolicLink((PUNICODE_STRING)&unicodeDosDeviceName, (PUNICODE_STRING)&unicodeDeviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    return status;
}


_Use_decl_annotations_
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->FileObject != NULL);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        //SetTokenInformations();
        //AdjustPrivileges();     
        ChangeToken();
        break;
    case IRP_MJ_CLOSE:

        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    // Save Status for return and complete Irp
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


EXTERN_C DRIVER_INITIALIZE DriverEntry;
//#pragma INITCODE
//#pragma alloc_text(INIT, DriverEntry)
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
EXTERN_C NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(RegistryPath);

    if (!KD_DEBUGGER_NOT_PRESENT) {
        KdBreakPoint();//__debugbreak();
    }

    //if (*InitSafeBootMode) {
    //    return STATUS_ACCESS_DENIED;
    //}

    PAGED_CODE();

    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL,
               "FILE:%s, LINE:%d, DATE:%s, TIME:%s.\r\n", __FILE__, __LINE__, __DATE__, __TIME__);

    DriverObject->DriverUnload = Unload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    g_TokenOffset = GetTokenOffsetInProcess();

    Status = CreateDeviceSecure(DriverObject);

    return Status;
}
