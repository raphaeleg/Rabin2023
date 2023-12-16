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
    void Begin(std::string_view name) noexcept;
    void End(std::string_view name) noexcept;
    void DumpOutputToBuffer() noexcept;
    void StoreInHistory(std::string_view name, float percent) noexcept;
    void GetFromHistory(std::string_view name, float* ave, float* min, float* max) noexcept;
    void Draw() noexcept;
};