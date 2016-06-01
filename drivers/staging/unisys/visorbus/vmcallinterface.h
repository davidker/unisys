/* Copyright (C) 2010 - 2015 UNISYS CORPORATION
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 */

#ifndef __IOMONINTF_H__
#define __IOMONINTF_H__

/*
* This file contains all structures needed to support the VMCALLs for IO
* Virtualization.  The VMCALLs are provided by Monitor and used by IO code
* running on IO Partitions.
*/
static inline unsigned long
__unisys_vmcall_gnuc(unsigned long tuple, unsigned long reg_ebx,
		     unsigned long reg_ecx)
{
	unsigned long result = 0;
	unsigned int cpuid_eax, cpuid_ebx, cpuid_ecx, cpuid_edx;

	cpuid(0x00000001, &cpuid_eax, &cpuid_ebx, &cpuid_ecx, &cpuid_edx);
	if (!(cpuid_ecx & 0x80000000))
		return -EPERM;

	__asm__ __volatile__(".byte 0x00f, 0x001, 0x0c1" : "=a"(result) :
		"a"(tuple), "b"(reg_ebx), "c"(reg_ecx));
	return result;
}

static inline unsigned long
__unisys_extended_vmcall_gnuc(unsigned long long tuple,
			      unsigned long long reg_ebx,
			      unsigned long long reg_ecx,
			      unsigned long long reg_edx)
{
	unsigned long result = 0;
	unsigned int cpuid_eax, cpuid_ebx, cpuid_ecx, cpuid_edx;

	cpuid(0x00000001, &cpuid_eax, &cpuid_ebx, &cpuid_ecx, &cpuid_edx);
	if (!(cpuid_ecx & 0x80000000))
		return -EPERM;

	__asm__ __volatile__(".byte 0x00f, 0x001, 0x0c1" : "=a"(result) :
		"a"(tuple), "b"(reg_ebx), "c"(reg_ecx), "d"(reg_edx));
	return result;
}

#ifdef VMCALL_IO_CONTROLVM_ADDR
#undef VMCALL_IO_CONTROLVM_ADDR
#endif	/*  */

/* define subsystem number for AppOS, used in uislib driver  */
#define MDS_APPOS 0x4000000000000000L	/* subsystem = 62 - AppOS */
enum vmcall_monitor_interface_method_tuple { /* VMCALL identification tuples  */
	    /* Note: when a new VMCALL is added:
	     * - the 1st 2 hex digits correspond to one of the
	     *   VMCALL_MONITOR_INTERFACE types and
	     * - the next 2 hex digits are the nth relative instance of within a
	     *   type
	     * E.G. for VMCALL_VIRTPART_RECYCLE_PART,
	     * - the 0x02 identifies it as a VMCALL_VIRTPART type and
	     * - the 0x01 identifies it as the 1st instance of a VMCALL_VIRTPART
	     *   type of VMCALL
	     */
	/* used by all Guests, not just IO */
	VMCALL_IO_CONTROLVM_ADDR = 0x0501,
	/* Allow caller to query virtual time offset */
	VMCALL_QUERY_GUEST_VIRTUAL_TIME_OFFSET = 0x0708,
	/* LOGEVENT Post Code (RDX) with specified subsystem mask */
	/* (RCX - monitor_subsystems.h) and severity (RDX) */
	VMCALL_POST_CODE_LOGEVENT = 0x070B,
	/* Allow ULTRA_SERVICE_CAPABILITY_TIME capable guest to make VMCALL */
	VMCALL_UPDATE_PHYSICAL_TIME = 0x0a02
};

#define VMCALL_SUCCESS 0
#define VMCALL_SUCCESSFUL(result)	(result == 0)

#define unisys_vmcall(tuple, reg_ebx, reg_ecx) \
	__unisys_vmcall_gnuc(tuple, reg_ebx, reg_ecx)
#define unisys_extended_vmcall(tuple, reg_ebx, reg_ecx, reg_edx) \
	__unisys_extended_vmcall_gnuc(tuple, reg_ebx, reg_ecx, reg_edx)
#define ISSUE_IO_VMCALL(method, param, result) \
	(result = unisys_vmcall(method, (param) & 0xFFFFFFFF,	\
				(param) >> 32))
#define ISSUE_IO_EXTENDED_VMCALL(method, param1, param2, param3) \
	unisys_extended_vmcall(method, param1, param2, param3)

    /* The following uses VMCALL_POST_CODE_LOGEVENT interface but is currently
     * not used much
     */
#define ISSUE_IO_VMCALL_POSTCODE_SEVERITY(postcode, severity)		\
	ISSUE_IO_EXTENDED_VMCALL(VMCALL_POST_CODE_LOGEVENT, severity,	\
				 MDS_APPOS, postcode)

/* Structures for IO VMCALLs */

/* Parameters to VMCALL_IO_CONTROLVM_ADDR interface */
struct vmcall_io_controlvm_addr_params {
	/* The Guest-relative physical address of the ControlVm channel. */
	/* This VMCall fills this in with the appropriate address. */
	u64 address;	/* contents provided by this VMCALL (OUT) */
	/* the size of the ControlVm channel in bytes This VMCall fills this */
	/* in with the appropriate address. */
	u32 channel_bytes;	/* contents provided by this VMCALL (OUT) */
	u8 unused[4];		/* Unused Bytes in the 64-Bit Aligned Struct */
} __packed;

#define POSTCODE_SEVERITY_ERR DIAG_SEVERITY_ERR
#define POSTCODE_SEVERITY_WARNING DIAG_SEVERITY_WARNING
/* TODO-> Info currently doesn't show, so we set info=warning */
#define POSTCODE_SEVERITY_INFO DIAG_SEVERITY_PRINT

/* Write a 64-bit value to the hypervisor's log file
 * POSTCODE_LINUX generates a value in the form 0xAABBBCCCDDDDEEEE where
 *	A is an identifier for the file logging the postcode
 *	B is an identifier for the event logging the postcode
 *	C is the line logging the postcode
 *	D is additional information the caller wants to log
 *	E is additional information the caller wants to log
 * Please also note that the resulting postcode is in hex, so if you are
 * searching for the __LINE__ number, convert it first to decimal.  The line
 * number combined with driver and type of call, will allow you to track down
 * exactly what line an error occurred on, or where the last driver
 * entered/exited from.
 */

#define POSTCODE_LINUX(EVENT_PC, pc16bit1, pc16bit2, severity)		\
do {									\
	unsigned long long post_code_temp;				\
	post_code_temp = (((u64)CURRENT_FILE_PC) << 56) |		\
		(((u64)EVENT_PC) << 44) |				\
		((((u64)__LINE__) & 0xFFF) << 32) |			\
		((((u64)pc16bit1) & 0xFFFF) << 16) |			\
		(((u64)pc16bit2) & 0xFFFF);				\
	ISSUE_IO_VMCALL_POSTCODE_SEVERITY(post_code_temp, severity);	\
} while (0)

#endif
