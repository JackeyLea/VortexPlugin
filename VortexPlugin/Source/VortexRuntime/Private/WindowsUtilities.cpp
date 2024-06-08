#include"WindowsUtilities.h"

#ifdef _WIN32
#undef TEXT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif 
#include <windows.h>
#endif

namespace
{
    // The maximum length of a value string we can fetch from the registry.
    const size_t MaxValueSize = 2048;
    const size_t MaxKeyLength = 256;
}


std::string WindowsUtilities::GetRegistryValue(bool CurrentUser, const std::string& Location, const std::string& Name, long* ErrorCode)
{
    HKEY hKey;
    CHAR Value[MaxValueSize];
    DWORD BufLen = MaxValueSize * sizeof(CHAR);
    long ret = E_FAIL;

    HKEY registry = (CurrentUser) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

    ret = RegOpenKeyExA(registry, Location.c_str(), 0, KEY_QUERY_VALUE, &hKey);
    if (ret == ERROR_SUCCESS)
    {
        ret = RegQueryValueExA(hKey, Name.c_str(), 0, 0, (LPBYTE)Value, &BufLen);
        RegCloseKey(hKey);
    }

    std::string stringValue;
    size_t pos = 0;
    if (ret == ERROR_SUCCESS)
    {
        if (BufLen <= MaxValueSize * sizeof(CHAR))
        {
            stringValue = std::string(Value, (size_t)BufLen - 1);
            pos = stringValue.length();
            while (pos > 0 && stringValue[pos - 1] == '\0')
            {
                --pos;
            }
        }
        else
        {
            ret = ERROR_BAD_LENGTH;
        }
    }

    if (ErrorCode != nullptr)
    {
        *ErrorCode = ret;
    }
    return stringValue.substr(0, pos);
}


std::vector<std::string> WindowsUtilities::GetSubKeys(bool CurrentUser, const std::string& Location, long* ErrorCode)
{
    HKEY hKey = 0;
    long ret = 0;
    std::vector<std::string> subKeys;

    HKEY registry = (CurrentUser) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    ret = RegOpenKeyExA(registry, Location.c_str(), 0, KEY_READ, &hKey);
    
    if (ret == ERROR_SUCCESS)
    {
        DWORD subKeysCount = 0;  
        // Get the sub key count
        ret = RegQueryInfoKeyA( hKey,           // key handle 
                                NULL,           // buffer for class name 
                                NULL,           // size of class string 
                                NULL,           // reserved 
                                &subKeysCount,  // number of subkeys 
                                NULL,           // longest subkey size 
                                NULL,           // longest class string 
                                NULL,           // number of values for this key 
                                NULL,           // longest value name 
                                NULL,           // longest value data 
                                NULL,           // security descriptor 
                                NULL);          // last write time 

                                     

        if (subKeysCount > 0)
        {
            subKeys.reserve(subKeysCount);

            for (DWORD i = 0; i < subKeysCount; i++)
            {
                CHAR        name[MaxKeyLength] = ("");   
                DWORD       nameSize = static_cast<DWORD>(MaxKeyLength);

                ret = RegEnumKeyExA(hKey, i, 
                                    name,
                                    &nameSize,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
                if (ret == ERROR_SUCCESS)
                {
                    subKeys.push_back(name);
                }
            }
        }
    }

    if (ErrorCode != nullptr)
    {
        *ErrorCode = ret;
    }

    return subKeys;
}


