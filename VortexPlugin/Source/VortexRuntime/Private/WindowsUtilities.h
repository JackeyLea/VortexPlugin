#pragma once
//Copyright(c) 2019 CM Labs Simulations Inc.All rights reserved.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//the sample code softwareand associated documentation files(the "Software"), to deal with
//the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
//of the Software, and to permit persons to whom the Software is furnished to
//do so, subject to the following conditions :
//
//Redistributions of source code must retain the above copyright notice,
//this list of conditionsand the following disclaimers.
//Redistributions in binary form must reproduce the above copyright notice,
//this list of conditionsand the following disclaimers in the documentation
//and /or other materials provided with the distribution.
//Neither the names of CM Labs or Vortex Studio
//nor the names of its contributors may be used to endorse or promote products
//derived from this Software without specific prior written permission.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
//SOFTWARE.
#include <string>
#include <vector>


/// This code iis providing some utility functions using the Windows API.
/// We encapsulate the implemenetation, because some MACROs are colliding with Unreal's ones.
///
namespace WindowsUtilities
{
    /// @internal Gets a value from the windows registry
    /// @param currentUser If true, searches in HKEY_CURRENT_USER registry, else HKEY_LOCAL_MACHINE registry
    /// @param location The location of the key.
    /// @param name The name of the value.
    /// @param errorCode If provided, this will contain the status code returned by windows.
    /// @return The registry value or empty if there is an error.
    ///
    std::string GetRegistryValue(bool CurrentUser, const std::string& Location, const std::string& Name, long* ErrorCode = nullptr);

    /// @internal Gets a list of sub keys from the windows registry.
    /// @param currentUser If true, searches in HKEY_CURRENT_USER registry, else HKEY_LOCAL_MACHINE registry
    /// @param location The location of the key.
    /// @param errorCode If provided, this will contain the status code returned by windows.
    /// @return A vector with all the sub keys at the given key
    ///
    std::vector<std::string> GetSubKeys(bool CurrentUser, const std::string& Location, long* ErrorCode = nullptr);
}

