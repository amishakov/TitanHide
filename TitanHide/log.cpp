#include "log.h"

static UNICODE_STRING LogFilename;
static wchar_t LogFilenameBuffer[256];

void InitLog(const PUNICODE_STRING DriverName)
{
    RtlInitEmptyUnicodeString(&LogFilename, LogFilenameBuffer, sizeof(LogFilenameBuffer));
    RtlAppendUnicodeToString(&LogFilename, L"\\DosDevices\\C:\\");
    RtlAppendUnicodeStringToString(&LogFilename, DriverName);
    RtlAppendUnicodeToString(&LogFilename, L".log");
    Log("[TITANHIDE] Log file initialized: %.*ws\r\n",
        LogFilename.Length / sizeof(WCHAR), LogFilename.Buffer);
}

void Log(const char* format, ...)
{
    char msg[1024] = "";
    va_list vl;
    va_start(vl, format);
    const int n = _vsnprintf(msg, sizeof(msg) / sizeof(char), format, vl);
    msg[n] = '\0';
    va_end(vl);
#ifdef _DEBUG
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, msg);
#endif
    va_end(format);
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &LogFilename,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    if(KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
#ifdef _DEBUG
        DbgPrint("[TITANHIDE] KeGetCurrentIrql != PASSIVE_LEVEL!\n");
#endif
        return;
    }
    HANDLE handle;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntstatus = ZwCreateFile(&handle,
                                     FILE_APPEND_DATA,
                                     &objAttr, &ioStatusBlock, NULL,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_WRITE | FILE_SHARE_READ,
                                     FILE_OPEN_IF,
                                     FILE_SYNCHRONOUS_IO_NONALERT,
                                     NULL, 0);
    if(NT_SUCCESS(ntstatus))
    {
        size_t cb;
        ntstatus = RtlStringCbLengthA(msg, sizeof(msg), &cb);
        if(NT_SUCCESS(ntstatus))
            ZwWriteFile(handle, NULL, NULL, NULL, &ioStatusBlock, msg, (ULONG)cb, NULL, NULL);
        ZwClose(handle);
    }
}
