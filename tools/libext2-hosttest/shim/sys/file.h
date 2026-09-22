/* Host shim: the object-type enum from libphoenix's sys/file.h. */
#ifndef _SHIM_SYS_FILE_H
#define _SHIM_SYS_FILE_H
enum { otDir = 0, otFile, otDev, otSymlink, otUnknown };
#endif
