/*
 * Copyright (c) 2014 VMware, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __VPORT_H_
#define __VPORT_H_ 1

#include "Switch.h"

#define OVS_MAX_DPPORTS             MAXUINT16
#define OVS_DPPORT_NUMBER_INVALID   OVS_MAX_DPPORTS
/*
 * The local port (0) is a reserved port, that is not allowed to be be
 * created by the netlink command vport add. On linux, this port is created
 * at netlink command datapath new. However, on windows, we do not need to
 * create it, and more, we shouldn't. The userspace attempts to create two
 * internal vports, the LOCAL port (0) and the internal port (with any other
 * port number). The non-LOCAL internal port is used in the userspace when it
 * requests the internal port.
 */
#define OVS_DPPORT_NUMBER_LOCAL    0

/*
 * A Vport, or Virtual Port, is a port on the OVS. It can be one of the
 * following types. Some of the Vports are "real" ports on the hyper-v switch,
 * and some are not:
 * - VIF port (VM's NIC)
 * - External Adapters (physical NIC)
 * - Internal Adapter (Virtual adapter exposed on the host).
 * - Tunnel ports created by OVS userspace.
 */

typedef enum {
    OVS_STATE_UNKNOWN,
    OVS_STATE_PORT_CREATED,
    OVS_STATE_NIC_CREATED,
    OVS_STATE_CONNECTED,
    OVS_STATE_PORT_TEAR_DOWN,
    OVS_STATE_PORT_DELETED,
} OVS_VPORT_STATE;

typedef struct _OVS_VPORT_STATS {
    UINT64 rxPackets;
    UINT64 txPackets;
    UINT64 rxBytes;
    UINT64 txBytes;
} OVS_VPORT_STATS;

typedef struct _OVS_VPORT_ERR_STATS {
    UINT64  rxErrors;
    UINT64  txErrors;
    UINT64  rxDropped;
    UINT64  txDropped;
} OVS_VPORT_ERR_STATS;

/* used for vport netlink commands. */
typedef struct _OVS_VPORT_FULL_STATS {
    OVS_VPORT_STATS;
    OVS_VPORT_ERR_STATS;
}OVS_VPORT_FULL_STATS;
/*
 * Each internal, external adapter or vritual adapter has
 * one vport entry. In addition, we have one vport for each
 * tunnel type, such as vxlan, gre, gre64
 */
typedef struct _OVS_VPORT_ENTRY {
    LIST_ENTRY             ovsNameLink;
    LIST_ENTRY             portIdLink;
    LIST_ENTRY             portNoLink;

    OVS_VPORT_STATE        ovsState;
    OVS_VPORT_TYPE         ovsType;
    OVS_VPORT_STATS        stats;
    OVS_VPORT_ERR_STATS    errStats;
    UINT32                 portNo;
    UINT32                 mtu;
    CHAR                   ovsName[OVS_MAX_PORT_NAME_LENGTH];
    UINT32                 ovsNameLen;

    PVOID                  priv;
    NDIS_SWITCH_PORT_ID    portId;
    NDIS_SWITCH_NIC_INDEX  nicIndex;
    UINT16                 numaNodeId;
    NDIS_SWITCH_PORT_STATE portState;
    NDIS_SWITCH_NIC_STATE  nicState;
    NDIS_SWITCH_PORT_TYPE  portType;

    UINT8                  permMacAddress[MAC_ADDRESS_LEN];
    UINT8                  currMacAddress[MAC_ADDRESS_LEN];
    UINT8                  vmMacAddress[MAC_ADDRESS_LEN];

    NDIS_SWITCH_PORT_NAME  hvPortName;
    IF_COUNTED_STRING      portFriendlyName;
    NDIS_SWITCH_NIC_NAME   nicName;
    NDIS_VM_NAME           vmName;
    GUID                   netCfgInstanceId;
    BOOLEAN                isExternal;
    UINT32                 upcallPid; /* netlink upcall port id */
} OVS_VPORT_ENTRY, *POVS_VPORT_ENTRY;

struct _OVS_SWITCH_CONTEXT;

POVS_VPORT_ENTRY
OvsFindVportByPortNo(struct _OVS_SWITCH_CONTEXT *switchContext,
                     UINT32 portNo);
POVS_VPORT_ENTRY
OvsFindVportByOvsName(struct _OVS_SWITCH_CONTEXT *switchContext,
                      CHAR *name, UINT32 length);
POVS_VPORT_ENTRY
OvsFindVportByHvName(POVS_SWITCH_CONTEXT switchContext, PSTR name);
POVS_VPORT_ENTRY
OvsFindVportByPortIdAndNicIndex(struct _OVS_SWITCH_CONTEXT *switchContext,
                                NDIS_SWITCH_PORT_ID portId,
                                NDIS_SWITCH_NIC_INDEX index);

NDIS_STATUS OvsAddConfiguredSwitchPorts(struct _OVS_SWITCH_CONTEXT *switchContext);
NDIS_STATUS OvsInitConfiguredSwitchNics(struct _OVS_SWITCH_CONTEXT *switchContext);

VOID OvsClearAllSwitchVports(struct _OVS_SWITCH_CONTEXT *switchContext);

NDIS_STATUS HvCreateNic(POVS_SWITCH_CONTEXT switchContext,
                        PNDIS_SWITCH_NIC_PARAMETERS nicParam);
NDIS_STATUS HvCreatePort(POVS_SWITCH_CONTEXT switchContext,
                         PNDIS_SWITCH_PORT_PARAMETERS portParam);
VOID HvTeardownPort(POVS_SWITCH_CONTEXT switchContext,
                    PNDIS_SWITCH_PORT_PARAMETERS portParam);
VOID HvDeletePort(POVS_SWITCH_CONTEXT switchContext,
                  PNDIS_SWITCH_PORT_PARAMETERS portParam);
VOID HvConnectNic(POVS_SWITCH_CONTEXT switchContext,
                  PNDIS_SWITCH_NIC_PARAMETERS nicParam);
VOID HvUpdateNic(POVS_SWITCH_CONTEXT switchContext,
                 PNDIS_SWITCH_NIC_PARAMETERS nicParam);
VOID HvDeleteNic(POVS_SWITCH_CONTEXT switchContext,
                 PNDIS_SWITCH_NIC_PARAMETERS nicParam);
VOID HvDisconnectNic(POVS_SWITCH_CONTEXT switchContext,
                     PNDIS_SWITCH_NIC_PARAMETERS nicParam);

static __inline BOOLEAN
OvsIsTunnelVportType(OVS_VPORT_TYPE ovsType)
{
    return ovsType == OVS_VPORT_TYPE_VXLAN ||
           ovsType == OVS_VPORT_TYPE_GRE ||
           ovsType == OVS_VPORT_TYPE_GRE64;
}

static __inline BOOLEAN
OvsIsInternalVportType(OVS_VPORT_TYPE ovsType)
{
    return ovsType == OVS_VPORT_TYPE_INTERNAL;
}

static __inline UINT32
OvsGetExternalMtu()
{
    return ((POVS_VPORT_ENTRY) OvsGetExternalVport())->mtu;
}

#endif /* __VPORT_H_ */
