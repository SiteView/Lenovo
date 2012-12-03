#ifndef __wlanlib_h__
#define __wlanlib_h__

#include <QtCore/QLibrary>
#define _WIN32_WINNT 0x0600
#include <Windows.h>
//#include <wlanapi.h>

#define L2_PROFILE_MAX_NAME_LENGTH     256

#define L2_NOTIFICATION_SOURCE_NONE                   0
#define L2_NOTIFICATION_SOURCE_DOT3_AUTO_CONFIG       0X00000001
#define L2_NOTIFICATION_SOURCE_SECURITY  		    0X00000002
#define L2_NOTIFICATION_SOURCE_ONEX                   0X00000004	
#define L2_NOTIFICATION_SOURCE_WLAN_ACM          0X00000008
#define L2_NOTIFICATION_SOURCE_WLAN_MSM          0X00000010
#define L2_NOTIFICATION_SOURCE_WLAN_SECURITY     0X00000020
#define L2_NOTIFICATION_SOURCE_WLAN_IHV          0X00000040
#define L2_NOTIFICATION_SOURCE_WLAN_HNWK         0X00000080

#define L2_NOTIFICATION_SOURCE_ALL          				0X0000FFFF
#define L2_NOTIFICATION_CODE_PUBLIC_BEGIN         0X00000000
#define L2_NOTIFICATION_CODE_GROUP_SIZE           0x00001000
#define L2_NOTIFICATION_CODE_V2_BEGIN             (L2_NOTIFICATION_CODE_PUBLIC_BEGIN+L2_NOTIFICATION_CODE_GROUP_SIZE)

// the types of notification
// compatible with L2_NOTIFICATION_SOURCE
#define WLAN_NOTIFICATION_SOURCE_NONE         L2_NOTIFICATION_SOURCE_NONE
#define WLAN_NOTIFICATION_SOURCE_ALL          L2_NOTIFICATION_SOURCE_ALL

#define WLAN_NOTIFICATION_SOURCE_ACM          L2_NOTIFICATION_SOURCE_WLAN_ACM
#define WLAN_NOTIFICATION_SOURCE_MSM          L2_NOTIFICATION_SOURCE_WLAN_MSM
#define WLAN_NOTIFICATION_SOURCE_SECURITY     L2_NOTIFICATION_SOURCE_WLAN_SECURITY
#define WLAN_NOTIFICATION_SOURCE_IHV          L2_NOTIFICATION_SOURCE_WLAN_IHV
#define WLAN_NOTIFICATION_SOURCE_HNWK         L2_NOTIFICATION_SOURCE_WLAN_HNWK
#define WLAN_NOTIFICATION_SOURCE_ONEX         L2_NOTIFICATION_SOURCE_ONEX

// This structure is the notification structure which needs to be filled in by each component 
// The interface guid is filled in only by the AC 
// The NotificationSource signifies the type above 
// and NotificationCode is one of the enum values 
typedef struct _L2_NOTIFICATION_DATA {
    DWORD NotificationSource;
    DWORD NotificationCode;
    GUID InterfaceGuid;
    DWORD dwDataSize;
    
    PVOID pData;

} L2_NOTIFICATION_DATA, *PL2_NOTIFICATION_DATA;

#define WLAN_MAX_NAME_LENGTH L2_PROFILE_MAX_NAME_LENGTH

typedef enum _WLAN_INTERFACE_STATE {
    wlan_interface_state_not_ready,
    wlan_interface_state_connected,
    wlan_interface_state_ad_hoc_network_formed,
    wlan_interface_state_disconnecting,
    wlan_interface_state_disconnected,
    wlan_interface_state_associating,
    wlan_interface_state_discovering,
    wlan_interface_state_authenticating
} WLAN_INTERFACE_STATE, *PWLAN_INTERFACE_STATE;

typedef enum _WLAN_ADHOC_NETWORK_STATE {
    wlan_adhoc_network_state_formed = 0,
    wlan_adhoc_network_state_connected
} WLAN_ADHOC_NETWORK_STATE, *PWLAN_ADHOC_NETWORK_STATE;

typedef L2_NOTIFICATION_DATA WLAN_NOTIFICATION_DATA, *PWLAN_NOTIFICATION_DATA;

typedef VOID (WINAPI *WLAN_NOTIFICATION_CALLBACK) (PWLAN_NOTIFICATION_DATA, PVOID);

typedef struct _WLAN_INTERFACE_INFO {
    GUID InterfaceGuid;
    WCHAR strInterfaceDescription[WLAN_MAX_NAME_LENGTH];
    WLAN_INTERFACE_STATE isState;
} WLAN_INTERFACE_INFO, *PWLAN_INTERFACE_INFO;

typedef struct _WLAN_INTERFACE_INFO_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_INTERFACE_INFO InterfaceInfo[1];
} WLAN_INTERFACE_INFO_LIST, *PWLAN_INTERFACE_INFO_LIST;

typedef enum _WLAN_OPCODE_VALUE_TYPE {
        wlan_opcode_value_type_query_only = 0,
        wlan_opcode_value_type_set_by_group_policy,
        wlan_opcode_value_type_set_by_user,
        wlan_opcode_value_type_invalid
} WLAN_OPCODE_VALUE_TYPE, *PWLAN_OPCODE_VALUE_TYPE;

typedef enum _WLAN_INTF_OPCODE {
    wlan_intf_opcode_autoconf_start = 0x000000000,
    wlan_intf_opcode_autoconf_enabled,
    wlan_intf_opcode_background_scan_enabled,
    wlan_intf_opcode_media_streaming_mode,
    wlan_intf_opcode_radio_state,
    wlan_intf_opcode_bss_type,
    wlan_intf_opcode_interface_state,
    wlan_intf_opcode_current_connection,
    wlan_intf_opcode_channel_number,
    wlan_intf_opcode_supported_infrastructure_auth_cipher_pairs,
    wlan_intf_opcode_supported_adhoc_auth_cipher_pairs,
    wlan_intf_opcode_supported_country_or_region_string_list,
    wlan_intf_opcode_current_operation_mode,
    wlan_intf_opcode_supported_safe_mode,
    wlan_intf_opcode_certified_safe_mode,
    wlan_intf_opcode_hosted_network_capable,
    wlan_intf_opcode_autoconf_end = 0x0fffffff,
    wlan_intf_opcode_msm_start = 0x10000100,
    wlan_intf_opcode_statistics,
    wlan_intf_opcode_rssi,
    wlan_intf_opcode_msm_end = 0x1fffffff,
    wlan_intf_opcode_security_start = 0x20010000,
    wlan_intf_opcode_security_end = 0x2fffffff,
    wlan_intf_opcode_ihv_start = 0x30000000,
    wlan_intf_opcode_ihv_end = 0x3fffffff
} WLAN_INTF_OPCODE, *PWLAN_INTF_OPCODE;

typedef enum _DOT11_RADIO_STATE {
    dot11_radio_state_unknown = 0,
    dot11_radio_state_on,
    dot11_radio_state_off
} DOT11_RADIO_STATE, *PDOT11_RADIO_STATE;

// the maximum number of PHYs supported by a NIC
#define WLAN_MAX_PHY_INDEX 64

typedef struct _WLAN_PHY_RADIO_STATE {
    DWORD dwPhyIndex;
    DOT11_RADIO_STATE dot11SoftwareRadioState;
    DOT11_RADIO_STATE dot11HardwareRadioState;
} WLAN_PHY_RADIO_STATE, *PWLAN_PHY_RADIO_STATE;

typedef struct _WLAN_RADIO_STATE {
    DWORD dwNumberOfPhys;
    WLAN_PHY_RADIO_STATE PhyRadioState[WLAN_MAX_PHY_INDEX];
} WLAN_RADIO_STATE, *PWLAN_RADIO_STATE;

typedef enum _WLAN_NOTIFICATION_ACM {
    wlan_notification_acm_start = L2_NOTIFICATION_CODE_PUBLIC_BEGIN,
    wlan_notification_acm_autoconf_enabled,
    wlan_notification_acm_autoconf_disabled,
    wlan_notification_acm_background_scan_enabled,
    wlan_notification_acm_background_scan_disabled,
    wlan_notification_acm_bss_type_change,
    wlan_notification_acm_power_setting_change,
    wlan_notification_acm_scan_complete,
    wlan_notification_acm_scan_fail,
    wlan_notification_acm_connection_start,
    wlan_notification_acm_connection_complete,
    wlan_notification_acm_connection_attempt_fail,
    wlan_notification_acm_filter_list_change,
    wlan_notification_acm_interface_arrival,
    wlan_notification_acm_interface_removal,
    wlan_notification_acm_profile_change,
    wlan_notification_acm_profile_name_change,
    wlan_notification_acm_profiles_exhausted,
    wlan_notification_acm_network_not_available,
    wlan_notification_acm_network_available,
    wlan_notification_acm_disconnecting,
    wlan_notification_acm_disconnected,
    wlan_notification_acm_adhoc_network_state_change,
    wlan_notification_acm_end
} WLAN_NOTIFICATION_ACM, *PWLAN_NOTIFICATION_ACM;

typedef enum _WLAN_CONNECTION_MODE {
    wlan_connection_mode_profile = 0,
    wlan_connection_mode_temporary_profile,
    wlan_connection_mode_discovery_secure,
    wlan_connection_mode_discovery_unsecure,
    wlan_connection_mode_auto,
    wlan_connection_mode_invalid
} WLAN_CONNECTION_MODE, *PWLAN_CONNECTION_MODE;

typedef enum _DOT11_BSS_TYPE {
    dot11_BSS_type_infrastructure = 1,
    dot11_BSS_type_independent = 2,
    dot11_BSS_type_any = 3
} DOT11_BSS_TYPE, * PDOT11_BSS_TYPE;

typedef enum _DOT11_PHY_TYPE {
    dot11_phy_type_unknown = 0,
    dot11_phy_type_any = dot11_phy_type_unknown,
    dot11_phy_type_fhss = 1,
    dot11_phy_type_dsss = 2,
    dot11_phy_type_irbaseband = 3,
    dot11_phy_type_ofdm = 4,
    dot11_phy_type_hrdsss = 5,
    dot11_phy_type_erp = 6,
    dot11_phy_type_ht = 7,
    dot11_phy_type_IHV_start = 0x80000000,
    dot11_phy_type_IHV_end = 0xffffffff
} DOT11_PHY_TYPE, * PDOT11_PHY_TYPE;

typedef enum _DOT11_AUTH_ALGORITHM {
    DOT11_AUTH_ALGO_80211_OPEN = 1,
    DOT11_AUTH_ALGO_80211_SHARED_KEY = 2,
    DOT11_AUTH_ALGO_WPA = 3,
    DOT11_AUTH_ALGO_WPA_PSK = 4,
    DOT11_AUTH_ALGO_WPA_NONE = 5,               // used in NatSTA only
    DOT11_AUTH_ALGO_RSNA = 6,
    DOT11_AUTH_ALGO_RSNA_PSK = 7,
    DOT11_AUTH_ALGO_IHV_START = 0x80000000,
    DOT11_AUTH_ALGO_IHV_END = 0xffffffff
} DOT11_AUTH_ALGORITHM, * PDOT11_AUTH_ALGORITHM;

typedef enum _DOT11_CIPHER_ALGORITHM {
    DOT11_CIPHER_ALGO_NONE = 0x00,
    DOT11_CIPHER_ALGO_WEP40 = 0x01,
    DOT11_CIPHER_ALGO_TKIP = 0x02,
    DOT11_CIPHER_ALGO_CCMP = 0x04,
    DOT11_CIPHER_ALGO_WEP104 = 0x05,
    DOT11_CIPHER_ALGO_WPA_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_RSN_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_WEP = 0x101,
    DOT11_CIPHER_ALGO_IHV_START = 0x80000000,
    DOT11_CIPHER_ALGO_IHV_END = 0xffffffff
} DOT11_CIPHER_ALGORITHM, * PDOT11_CIPHER_ALGORITHM;

#define DOT11_SSID_MAX_LENGTH   32      // 32 bytes
typedef struct _DOT11_SSID {
    ULONG uSSIDLength;
    UCHAR ucSSID[DOT11_SSID_MAX_LENGTH];
} DOT11_SSID, * PDOT11_SSID;

typedef UCHAR DOT11_MAC_ADDRESS[6];
typedef DOT11_MAC_ADDRESS * PDOT11_MAC_ADDRESS;

typedef ULONG WLAN_SIGNAL_QUALITY, *PWLAN_SIGNAL_QUALITY; 

typedef struct _WLAN_ASSOCIATION_ATTRIBUTES {
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    DOT11_MAC_ADDRESS dot11Bssid;
    DOT11_PHY_TYPE dot11PhyType;
    ULONG uDot11PhyIndex;
    WLAN_SIGNAL_QUALITY wlanSignalQuality;
    ULONG ulRxRate;
    ULONG ulTxRate;
} WLAN_ASSOCIATION_ATTRIBUTES, *PWLAN_ASSOCIATION_ATTRIBUTES;

typedef struct _WLAN_SECURITY_ATTRIBUTES {
    BOOL bSecurityEnabled;
    BOOL bOneXEnabled;
    DOT11_AUTH_ALGORITHM dot11AuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11CipherAlgorithm;
} WLAN_SECURITY_ATTRIBUTES, *PWLAN_SECURITY_ATTRIBUTES;

typedef struct _WLAN_CONNECTION_ATTRIBUTES {
    WLAN_INTERFACE_STATE isState;
    WLAN_CONNECTION_MODE wlanConnectionMode;
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    WLAN_ASSOCIATION_ATTRIBUTES wlanAssociationAttributes;
    WLAN_SECURITY_ATTRIBUTES wlanSecurityAttributes;
} WLAN_CONNECTION_ATTRIBUTES, *PWLAN_CONNECTION_ATTRIBUTES;

typedef DWORD WLAN_REASON_CODE, *PWLAN_REASON_CODE;

#define WLAN_MAX_PHY_TYPE_NUMBER    8

typedef struct _WLAN_AVAILABLE_NETWORK {
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    ULONG uNumberOfBssids;
    BOOL bNetworkConnectable;
    WLAN_REASON_CODE wlanNotConnectableReason;
    ULONG uNumberOfPhyTypes;
    DOT11_PHY_TYPE dot11PhyTypes[WLAN_MAX_PHY_TYPE_NUMBER];
    // bMorePhyTypes is set to TRUE if the PHY types for the network
    // exceeds WLAN_MAX_PHY_TYPE_NUMBER.
    // In this case, uNumerOfPhyTypes is WLAN_MAX_PHY_TYPE_NUMBER and the
    // first WLAN_MAX_PHY_TYPE_NUMBER PHY types are returned.
    BOOL bMorePhyTypes;
    WLAN_SIGNAL_QUALITY wlanSignalQuality;
    BOOL bSecurityEnabled;
    DOT11_AUTH_ALGORITHM dot11DefaultAuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11DefaultCipherAlgorithm;
    DWORD dwFlags;
    DWORD dwReserved;
} WLAN_AVAILABLE_NETWORK, *PWLAN_AVAILABLE_NETWORK;

typedef struct _WLAN_AVAILABLE_NETWORK_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
    WLAN_AVAILABLE_NETWORK Network[1];
} WLAN_AVAILABLE_NETWORK_LIST, *PWLAN_AVAILABLE_NETWORK_LIST;

#define DOT11_RATE_SET_MAX_LENGTH               126 // 126 bytes

typedef struct _WLAN_RATE_SET {
    ULONG uRateSetLength;
    USHORT usRateSet[DOT11_RATE_SET_MAX_LENGTH];
} WLAN_RATE_SET, * PWLAN_RATE_SET;

typedef struct _WLAN_BSS_ENTRY {
    DOT11_SSID dot11Ssid;
    ULONG uPhyId;
    DOT11_MAC_ADDRESS dot11Bssid;
    DOT11_BSS_TYPE dot11BssType;
    DOT11_PHY_TYPE dot11BssPhyType;
    LONG lRssi;
    ULONG uLinkQuality;
    BOOLEAN bInRegDomain;
    USHORT usBeaconPeriod;
    ULONGLONG ullTimestamp;
    ULONGLONG ullHostTimestamp;
    USHORT usCapabilityInformation;
    ULONG  ulChCenterFrequency;
    WLAN_RATE_SET wlanRateSet;
    // the beginning of the IE blob
    // the offset is w.r.t. the beginning of the entry
    ULONG ulIeOffset;
    // size of the IE blob
    ULONG ulIeSize;
} WLAN_BSS_ENTRY, * PWLAN_BSS_ENTRY;

// struct WLAN_VARIABLE_SIZE_ARRAY defines a list of entries,
// each of which may have different size  
typedef struct _WLAN_BSS_LIST {
    // The total size of the data in BYTE
    DWORD dwTotalSize;
    DWORD dwNumberOfItems;
    WLAN_BSS_ENTRY wlanBssEntries[1];
} WLAN_BSS_LIST, *PWLAN_BSS_LIST;

#define WLAN_AVAILABLE_NETWORK_CONNECTED                    0x00000001  // This network is currently connected
#define WLAN_AVAILABLE_NETWORK_HAS_PROFILE                  0x00000002  // There is a profile for this network
#define WLAN_AVAILABLE_NETWORK_CONSOLE_USER_PROFILE         0x00000004  // The profile is the active console user's per user profile

typedef struct _NDIS_OBJECT_HEADER
{
    UCHAR   Type;
    UCHAR   Revision;
    USHORT  Size;
} NDIS_OBJECT_HEADER, *PNDIS_OBJECT_HEADER;

#define NDIS_OBJECT_TYPE_DEFAULT                            0x80    // used when object type is implicit in the API call

typedef struct DOT11_BSSID_LIST {
    #define DOT11_BSSID_LIST_REVISION_1  1
    NDIS_OBJECT_HEADER Header;
    ULONG uNumOfEntries;
    ULONG uTotalNumOfEntries;
    DOT11_MAC_ADDRESS BSSIDs[1];
} DOT11_BSSID_LIST, * PDOT11_BSSID_LIST;

typedef struct _WLAN_CONNECTION_PARAMETERS {
    WLAN_CONNECTION_MODE wlanConnectionMode;
    LPCWSTR strProfile;
    PDOT11_SSID pDot11Ssid;
    PDOT11_BSSID_LIST pDesiredBssidList;
    DOT11_BSS_TYPE dot11BssType;
    DWORD dwFlags;
} WLAN_CONNECTION_PARAMETERS, *PWLAN_CONNECTION_PARAMETERS;

typedef struct _WLAN_CONNECTION_NOTIFICATION_DATA {
    WLAN_CONNECTION_MODE wlanConnectionMode;
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    BOOL bSecurityEnabled;
    WLAN_REASON_CODE wlanReasonCode;
    DWORD dwFlags;
    WCHAR strProfileXml[1];
} WLAN_CONNECTION_NOTIFICATION_DATA, *PWLAN_CONNECTION_NOTIFICATION_DATA;

class WlanLib
{
public:
	WlanLib();
	~WlanLib();

	bool init();

#undef WLANAPI_ENTRY
#define WLANAPI_ENTRY(r, n, p) typedef r (WINAPI *PFN_ ## n) p; PFN_ ## n n;
#include "wlandef.h"
#undef WLANAPI_ENTRY

private:
	QLibrary m_libWlanApi;
};

#endif // __wlanlib_h__
