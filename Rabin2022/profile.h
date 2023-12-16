/* Copyright (C) Steve Rabin, 2000. 
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Steve Rabin, 2000"
 */


#pragma once
#include <string_view>

namespace Profile {
    void Init() noexcept;
    void Begin(const char* name) noexcept;
    void End(const char* name) noexcept;
    void DumpOutputToBuffer() noexcept;
    void StoreInHistory(const char* name, float percent) noexcept;
    void GetFromHistory(const char* name, float* ave, float* min, float* max) noexcept;
    void Draw() noexcept;
};