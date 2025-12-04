#pragma once
#include <string>

namespace sqlparser {
    // UNICODE対応のため wchar_t を使用
    using String = std::wstring;
    using Char = wchar_t;
}
