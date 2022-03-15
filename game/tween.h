#pragma once

#include <glm/glm.hpp>

namespace Tweeners {

namespace Detail {

template<typename T, typename F>
struct Out {
    T operator()(T t) const
    {
        return 1.f - F()(1.f - t);
    }
};

template<typename T, typename F>
struct InOut {
    T operator()(T t) const
    {
        if (t < .5f) {
            F f;
            return .5f * f(2.f * t);
        } else {
            Out<T, F> f;
            return .5f + .5f * f(2.f * t - 1);
        }
    }
};

} // namespace Detail

template<typename T>
struct Linear {
    T operator()(T t) const
    {
        return t;
    }
};

template<typename T>
struct InQuadratic {
    T operator()(T t) const
    {
        return t * t;
    }
};

template<typename T>
using OutQuadratic = Detail::Out<T, InQuadratic<T>>;

template<typename T>
using InOutQuadratic = Detail::InOut<T, InQuadratic<T>>;

template<typename T>
struct InBack {
    T operator()(T t) const
    {
        // stolen from robert penner
        constexpr const auto BackS = 1.70158f;
        return t * t * ((BackS + 1.f) * t - BackS);
    }
};

template<typename T>
using OutBack = Detail::Out<T, InBack<T>>;

template<typename T>
using InOutBack = Detail::InOut<T, InBack<T>>;

template<typename T>
struct OutBounce {
    T operator()(T t) const
    {
        if (t < 1. / 2.75) {
            return 7.5625 * t * t;
        } else if (t < 2. / 2.75) {
            t -= 1.5 / 2.75;
            return 7.5625 * t * t + .75;
        } else if (t < 2.5 / 2.75) {
            t -= 2.25 / 2.75;
            return 7.5625 * t * t + .9375;
        } else {
            t -= 2.625 / 2.75;
            return 7.5625 * t * t + .984375;
        }
    }
};

template<typename T>
using InBounce = Detail::Out<T, OutBounce<T>>;

template<typename T>
using InOutBounce = Detail::InOut<T, InBounce<T>>;

} // namespace Tweeners

template<typename F, typename T, typename U>
inline T tween(T x, T y, U t)
{
    const F tweener;
    return glm::mix(x, y, tweener(t));
}
