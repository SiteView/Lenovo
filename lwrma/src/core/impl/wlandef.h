#ifdef WLANAPI_ENTRY
WLANAPI_ENTRY(DWORD, WlanOpenHandle, (DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle))
WLANAPI_ENTRY(DWORD, WlanCloseHandle, (HANDLE hClientHandle, PVOID pReserved))
WLANAPI_ENTRY(DWORD, WlanEnumInterfaces, (HANDLE hClientHandle, PVOID pReserved, PWLAN_INTERFACE_INFO_LIST *ppInterfaceList))
WLANAPI_ENTRY(DWORD, WlanRegisterNotification, (HANDLE hClientHandle, DWORD dwNotifSource, BOOL bIgnoreDuplicate, WLAN_NOTIFICATION_CALLBACK funcCallback, PVOID pCallbackContext, PVOID pReserved, PDWORD pdwPrevNotifSource))
WLANAPI_ENTRY(VOID, WlanFreeMemory, (PVOID pMemory))
WLANAPI_ENTRY(DWORD, WlanQueryInterface, (HANDLE hClientHandle, const GUID *pInterfaceGuid, WLAN_INTF_OPCODE OpCode, PVOID pReserved, PDWORD pdwDataSize, PVOID *ppData, PWLAN_OPCODE_VALUE_TYPE pWlanOpcodeValueType))
WLANAPI_ENTRY(DWORD, WlanGetAvailableNetworkList, (HANDLE hClientHandle, const GUID *pInterfaceGuid, DWORD dwFlags, PVOID pReserved, PWLAN_AVAILABLE_NETWORK_LIST *ppAvailableNetworkList))
WLANAPI_ENTRY(DWORD, WlanGetNetworkBssList, (HANDLE hClientHandle, const GUID *pInterfaceGuid, const  PDOT11_SSID pDot11Ssid, DOT11_BSS_TYPE dot11BssType, BOOL bSecurityEnabled, PVOID pReserved, PWLAN_BSS_LIST *ppWlanBssList))
WLANAPI_ENTRY(DWORD, WlanGetProfile, (HANDLE hClientHandle, const GUID *pInterfaceGuid, LPCWSTR strProfileName, PVOID pReserved, LPWSTR *pstrProfileXml, DWORD *pdwFlags, PDWORD pdwGrantedAccess))
WLANAPI_ENTRY(DWORD, WlanConnect, (HANDLE hClientHandle, const GUID *pInterfaceGuid, const PWLAN_CONNECTION_PARAMETERS pConnectionParameters, PVOID pReserved))
WLANAPI_ENTRY(DWORD, WlanSetProfile, (HANDLE hClientHandle, const GUID *pInterfaceGuid, DWORD dwFlags, LPCWSTR strProfileXml, LPCWSTR strAllUserProfileSecurity, BOOL bOverwrite, PVOID pReserved, DWORD *pdwReasonCode))
WLANAPI_ENTRY(DWORD, WlanReasonCodeToString, (DWORD dwReasonCode, DWORD dwBufferSize, PWCHAR pStringBuffer, PVOID pReserved))
#endif // WLANAPI_ENTRY
