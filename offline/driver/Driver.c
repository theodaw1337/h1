#include <ntddk.h>
#include <wdmsec.h>
#include "../Protocol.h"
C_ASSERT(sizeof(LAB_ACTOR)==48);
C_ASSERT(sizeof(LAB_SNAPSHOT)==1552);

typedef struct LAB_EXTENSION { KSPIN_LOCK lock; LAB_SNAPSHOT snapshot; } LAB_EXTENSION;
static const GUID LabClass = {0xf7f0d23a,0xdbf6,0x45a2,{0x89,0x32,0x17,0x9a,0x66,0x74,0x80,0x15}};
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD LabUnload;
DRIVER_DISPATCH LabDispatch;

NTSTATUS LabDispatch(PDEVICE_OBJECT device,PIRP irp) {
    PIO_STACK_LOCATION stack=IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status=STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR information=0;
    if (stack->MajorFunction==IRP_MJ_CREATE || stack->MajorFunction==IRP_MJ_CLOSE ||
        stack->MajorFunction==IRP_MJ_CLEANUP) status=STATUS_SUCCESS;
    else if (stack->MajorFunction==IRP_MJ_DEVICE_CONTROL) {
        LAB_EXTENSION* extension=(LAB_EXTENSION*)device->DeviceExtension;
        KIRQL previous;
        uint32_t written=0;
        LAB_RESULT result;
        KeAcquireSpinLock(&extension->lock,&previous);
        result=LabExchange(&extension->snapshot,stack->Parameters.DeviceIoControl.IoControlCode,
            irp->AssociatedIrp.SystemBuffer,stack->Parameters.DeviceIoControl.InputBufferLength,
            stack->Parameters.DeviceIoControl.OutputBufferLength,&written);
        KeReleaseSpinLock(&extension->lock,previous);
        switch (result) {
        case LabOk: status=STATUS_SUCCESS; information=written; break;
        case LabSmallBuffer: status=STATUS_BUFFER_TOO_SMALL; break;
        case LabInvalid: status=STATUS_INVALID_PARAMETER; break;
        default: status=STATUS_INVALID_DEVICE_REQUEST; break;
        }
    }
    irp->IoStatus.Status=status;
    irp->IoStatus.Information=information;
    IoCompleteRequest(irp,IO_NO_INCREMENT);
    return status;
}
VOID LabUnload(PDRIVER_OBJECT driver) {
    UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\H1OfflineLab");
    IoDeleteSymbolicLink(&link);
    if (driver->DeviceObject) IoDeleteDevice(driver->DeviceObject);
}
NTSTATUS DriverEntry(PDRIVER_OBJECT driver,PUNICODE_STRING registryPath) {
    UNICODE_STRING name=RTL_CONSTANT_STRING(L"\\Device\\H1OfflineLab");
    UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\H1OfflineLab");
    PDEVICE_OBJECT device=NULL;
    NTSTATUS status;
    ULONG i;
    UNREFERENCED_PARAMETER(registryPath);
    for (i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;++i) driver->MajorFunction[i]=LabDispatch;
    driver->DriverUnload=LabUnload;
    status=IoCreateDeviceSecure(driver,sizeof(LAB_EXTENSION),&name,LAB_DEVICE_TYPE,
        FILE_DEVICE_SECURE_OPEN,FALSE,&SDDL_DEVOBJ_SYS_ALL_ADM_ALL,&LabClass,&device);
    if (!NT_SUCCESS(status)) return status;
    KeInitializeSpinLock(&((LAB_EXTENSION*)device->DeviceExtension)->lock);
    LabInitialize(&((LAB_EXTENSION*)device->DeviceExtension)->snapshot);
    status=IoCreateSymbolicLink(&link,&name);
    if (!NT_SUCCESS(status)) { IoDeleteDevice(device); return status; }
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}
