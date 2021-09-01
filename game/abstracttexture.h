#pragma once

#include "noncopyable.h"

namespace GX {

class AbstractTexture : private NonCopyable
{
public:
    virtual ~AbstractTexture() = default;

    virtual void bind() const = 0;
};

} // namespace GX
