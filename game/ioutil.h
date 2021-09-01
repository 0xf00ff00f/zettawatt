#pragma once

#include <optional>
#include <string>
#include <vector>

namespace GX {
namespace Util {

std::optional<std::vector<unsigned char>> readFile(const std::string &path);

}
} // namespace GX
