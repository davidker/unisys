/* Copyright (C) 2010 - 2013 UNISYS CORPORATION
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 */

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <linux/types.h>
#include <linux/io.h>
#include <linux/uuid.h>

#define __SUPERVISOR_CHANNEL_H__

#define SIGNATURE_16(A, B) ((A) | ((B) << 8))
#define SIGNATURE_32(A, B, C, D) \
	(SIGNATURE_16(A, B) | (SIGNATURE_16(C, D) << 16))
#define SIGNATURE_64(A, B, C, D, E, F, G, H) \
	(SIGNATURE_32(A, B, C, D) | ((u64)(SIGNATURE_32(E, F, G, H)) << 32))

#ifndef COVER
#define COVER(v, d) ((d) * DIV_ROUND_UP(v, d))
#endif

#define VISOR_CHANNEL_SIGNATURE SIGNATURE_32('E', 'C', 'N', 'L')

enum channel_serverstate {
	/* Channel is in an undefined state. */
	CHANNELSRV_UNINITIALIZED = 0,
	/* Channel has been initialized by server. */
	CHANNELSRV_READY = 1
};

enum channel_clientstate {
	CHANNELCLI_DETACHED = 0,
	/* Client can see channel but is NOT allowed to use it unless given TBD
	 * explicit request (should actually be < DETACHED).
	 */
	CHANNELCLI_DISABLED = 1,
	/* Legacy EFI client request for EFI server to attach. */
	CHANNELCLI_ATTACHING = 2,
	/* Idle, but client may want to use channel any time. */
	CHANNELCLI_ATTACHED = 3,
	/* Client either wants to use or is using channel. */
	CHANNELCLI_BUSY = 4,
	/* "No worries" state - client can access channel anytime. */
	CHANNELCLI_OWNED = 5
};

#define VISOR_CHANNEL_SERVER_READY(ch) \
	(readl(&(ch)->srv_state) == CHANNELSRV_READY)

#define VISOR_VALID_CHANNELCLI_TRANSITION(o, n) \
	(((((o) == CHANNELCLI_DETACHED) && ((n) == CHANNELCLI_DISABLED)) || \
	  (((o) == CHANNELCLI_ATTACHING) && ((n) == CHANNELCLI_DISABLED)) || \
	  (((o) == CHANNELCLI_ATTACHED) && ((n) == CHANNELCLI_DISABLED)) || \
	  (((o) == CHANNELCLI_ATTACHING) && ((n) == CHANNELCLI_DETACHED)) || \
	  (((o) == CHANNELCLI_ATTACHED) && ((n) == CHANNELCLI_DETACHED)) || \
	  (((o) == CHANNELCLI_DETACHED) && ((n) == CHANNELCLI_ATTACHING)) || \
	  (((o) == CHANNELCLI_ATTACHING) && ((n) == CHANNELCLI_ATTACHED)) || \
	  (((o) == CHANNELCLI_DETACHED) && ((n) == CHANNELCLI_ATTACHED)) || \
	  (((o) == CHANNELCLI_BUSY) && ((n) == CHANNELCLI_ATTACHED)) ||	\
	  (((o) == CHANNELCLI_ATTACHED) && ((n) == CHANNELCLI_BUSY)) ||	\
	  (((o) == CHANNELCLI_DETACHED) && ((n) == CHANNELCLI_OWNED)) || \
	  (((o) == CHANNELCLI_DISABLED) && ((n) == CHANNELCLI_OWNED)) || \
	  (((o) == CHANNELCLI_ATTACHING) && ((n) == CHANNELCLI_OWNED)) || \
	  (((o) == CHANNELCLI_ATTACHED) && ((n) == CHANNELCLI_OWNED)) || \
	  (((o) == CHANNELCLI_BUSY) && ((n) == CHANNELCLI_OWNED)) || (0)) \
	 ? (1) : (0))

/* Values for VISORA_CHANNEL_PROTOCOL.CliErrorBoot: */
/* Throttling invalid boot channel statetransition error due to client
 * disabled
 */
#define VISOR_CLIERRORBOOT_THROTTLEMSG_DISABLED 0x01

/* Throttling invalid boot channel statetransition error due to client
 * not attached
 */
#define VISOR_CLIERRORBOOT_THROTTLEMSG_NOTATTACHED 0x02

/* Throttling invalid boot channel statetransition error due to busy channel */
#define VISOR_CLIERRORBOOT_THROTTLEMSG_BUSY 0x04

/* Values for VISOR_CHANNEL_PROTOCOL.Features: This define exists so
 * that windows guest can look at the FeatureFlags in the io channel,
 * and configure the windows driver to use interrupts or not based on
 * this setting.  This flag is set in uislib after the
 * VISOR_VHBA_init_channel is called.  All feature bits for all
 * channels should be defined here.  The io channel feature bits are
 * defined right here
 */
#define VISOR_DRIVER_ENABLES_INTS (0x1ULL << 1)
#define VISOR_CHANNEL_IS_POLLING (0x1ULL << 3)
#define VISOR_IOVM_OK_DRIVER_DISABLING_INTS (0x1ULL << 4)
#define VISOR_DRIVER_DISABLES_INTS (0x1ULL << 5)
#define VISOR_DRIVER_ENHANCED_RCVBUF_CHECKING (0x1ULL << 6)

/* Common Channel Header */
struct channel_header {
	/* Signature */
	u64 signature;
	/* DEPRECATED - being replaced by */
	u32 legacy_state;
	/* SrvState, CliStateBoot, and CliStateOS below */
	/* sizeof(struct channel_header) */
	u32 header_size;
	/* Total size of this channel in bytes */
	u64 size;
	/* Flags to modify behavior */
	u64 features;
	/* Channel type: data, bus, control, etc. */
	uuid_le chtype;
	/* ID of guest partition */
	u64 partition_handle;
	/* Device number of this channel in client */
	u64 handle;
	/* Offset in bytes to channel specific area */
	u64 ch_space_offset;
	/* Struct channel_header Version ID */
	u32 version_id;
	/* Index of guest partition */
	u32 partition_index;
	/* Guid of Channel's zone */
	uuid_le zone_uuid;
	/* Offset from channel header to null-terminated ClientString
	 * (0 if * ClientString not present)
	 */
	u32 cli_str_offset;
	/* CHANNEL_CLIENTSTATE of pre-boot EFI client of this channel */
	u32 cli_state_boot;
	/* CHANNEL_COMMANDSTATE (overloaded in Windows drivers,
	 * see ServerStateUp, ServerStateDown, etc)
	 */
	u32 cmd_state_cli;
	/* CHANNEL_CLIENTSTATE of Guest OS client of this channel */
	u32 cli_state_os;
	/* CHANNEL_CHARACTERISTIC_<xxx> */
	u32 ch_characteristic;
	/* CHANNEL_COMMANDSTATE (overloaded in Windows drivers,
	 * see ServerStateUp,* ServerStateDown, etc)
	 */
	u32 cmd_state_srv;
	/* CHANNEL_SERVERSTATE */
	u32 srv_state;
	/* Bits to indicate err states for boot clients, so err messages can be
	 * throttled.
	 */
	u8 cli_error_boot;
	/* Bits to indicate err states for OS clients, so err messages can be
	 * throttled.
	 */
	u8 cli_error_os;
	/* Pad out to 128 byte cacheline */
	u8 filler[1];
	/* Please add all new single-byte values below here */
	u8 recover_channel;
} __packed;

#define VISOR_CHANNEL_ENABLE_INTS (0x1ULL << 0)

/* Subheader for the Signal Type variation of the Common Channel */
struct signal_queue_header {
	/* 1st cache line */
	/* SIGNAL_QUEUE_HEADER Version ID */
	u32 version;
	/* Queue type: storage, network */
	u32 chtype;
	/* Total size of this queue in bytes */
	u64 size;
	/* Offset to signal queue area */
	u64 sig_base_offset;
	/* Flags to modify behavior */
	u64 features;
	/* Total # of signals placed in this queue */
	u64 num_sent;
	/* Total # of inserts failed due to full queue */
	u64 num_overflows;
	/* Total size of a signal for this queue */
	u32 signal_size;
	/* Max # of slots in queue, 1 slot is always empty */
	u32 max_slots;
	/* Max # of signals in queue (MaxSignalSlots-1) */
	u32 max_signals;
	/* Queue head signal # */
	u32 head;
	/* 2nd cache line */
	/* Total # of signals removed from this queue */
	u64 num_received;
	/* Queue tail signal */
	u32 tail;
	/* Reserved field */
	u32 reserved1;
	/* Reserved field */
	u64 reserved2;
	u64 client_queue;
	/* Total # of Interrupts received. This	is incremented by the ISR in the
	 * guest windows driver.
	 */
	u64 num_irq_received;
	/* Number of times that visor_signal_remove is called and returned
	 * Empty Status.
	 */
	u64 num_empty;
	/* Error bits set during SignalReinit to denote trouble with client's
	 * fields.
	 */
	u32 errorflags;
	/* Pad out to 64 byte cacheline. */
	u8 filler[12];
} __packed;

/* Generic function useful for validating any type of channel when it is
 * received by the client that will be accessing the channel.
 * Note that <logCtx> is only needed for callers in the EFI environment, and
 * is used to pass the EFI_DIAG_CAPTURE_PROTOCOL needed to log messages.
 */
static inline int
visor_check_channel(struct channel_header *ch,
		    uuid_le expected_uuid,
		    char *chname,
		    u64 expected_min_bytes,
		    u32 expected_version,
		    u64 expected_signature)
{
	if (uuid_le_cmp(expected_uuid, NULL_UUID_LE) != 0) {
		/* caller wants us to verify type GUID */
		if (uuid_le_cmp(ch->chtype, expected_uuid) != 0) {
			pr_err("Channel mismatch on channel=%s(%pUL) field=type expected=%pUL actual=%pUL\n",
			       chname, &expected_uuid,
			       &expected_uuid, &ch->chtype);
			return 0;
		}
	}
	/* verify channel size */
	if (expected_min_bytes > 0) {
		if (ch->size < expected_min_bytes) {
			pr_err("Channel mismatch on channel=%s(%pUL) field=size expected=0x%-8.8Lx actual=0x%-8.8Lx\n",
			       chname, &expected_uuid,
			       (unsigned long long)expected_min_bytes,
			       ch->size);
			return 0;
		}
	}
	/* verify channel version */
	if (expected_version > 0) {
		if (ch->version_id != expected_version) {
			pr_err("Channel mismatch on channel=%s(%pUL) field=version expected=0x%-8.8lx actual=0x%-8.8x\n",
			       chname, &expected_uuid,
			       (unsigned long)expected_version,
			       ch->version_id);
			return 0;
		}
	}
	/* verify channel signature */
	if (expected_signature > 0) {
		if (ch->signature != expected_signature) {
			pr_err("Channel mismatch on channel=%s(%pUL) field=signature expected=0x%-8.8Lx actual=0x%-8.8Lx\n",
			       chname, &expected_uuid,
			       expected_signature, ch->signature);
			return 0;
		}
	}
	return 1;
}

/*
 * CHANNEL Guids
 */

/* {414815ed-c58c-11da-95a9-00e08161165f} */
#define VISOR_VHBA_CHANNEL_UUID \
	UUID_LE(0x414815ed, 0xc58c, 0x11da, \
		0x95, 0xa9, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f)
static const uuid_le visor_vhba_channel_uuid = VISOR_VHBA_CHANNEL_UUID;
#define VISOR_VHBA_CHANNEL_UUID_STR \
	"414815ed-c58c-11da-95a9-00e08161165f"

/* {8cd5994d-c58e-11da-95a9-00e08161165f} */
#define VISOR_VNIC_CHANNEL_UUID \
	UUID_LE(0x8cd5994d, 0xc58e, 0x11da, \
		0x95, 0xa9, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f)
static const uuid_le visor_vnic_channel_uuid = VISOR_VNIC_CHANNEL_UUID;
#define VISOR_VNIC_CHANNEL_UUID_STR \
	"8cd5994d-c58e-11da-95a9-00e08161165f"

/* {72120008-4AAB-11DC-8530-444553544200} */
#define VISOR_SIOVM_UUID \
	UUID_LE(0x72120008, 0x4AAB, 0x11DC, \
		0x85, 0x30, 0x44, 0x45, 0x53, 0x54, 0x42, 0x00)
static const uuid_le visor_siovm_uuid = VISOR_SIOVM_UUID;

#endif
