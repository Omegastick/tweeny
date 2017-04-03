/*
 This file is part of the Tweeny library.

 Copyright (c) 2016-2017 Leonardo G. Lucena de Freitas
 Copyright (c) 2016 Guilherme R. Costa

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * The purpose of this file is to hold implementations for the tween.h file, s
 * pecializing on the single value case.
 */
#ifndef TWEENY_TWEENONE_TCC
#define TWEENY_TWEENONE_TCC

#include "tween.h"
#include "dispatcher.h"

namespace tweeny {
    template<typename T> inline tween<T> tween<T>::from(T t) { return tween<T>(t); }
    template<typename T> inline tween<T>::tween() { }
    template<typename T> inline tween<T>::tween(T t) {
        points.emplace_back(t);
    }

    template<typename T> inline tween<T> & tween<T>::to(T t) {
        points.emplace_back(t);
        return *this;
    }

    template<typename T>
    template<typename... Fs>
    inline tween<T> & tween<T>::via(Fs... vs) {
        points.at(points.size() - 2).via(vs...);
        return *this;
    }

    template<typename T>
    template<typename... Fs>
    inline tween<T> & tween<T>::via(int index, Fs... vs) {
        points.at(static_cast<size_t>(index)).via(vs...);
        return *this;
    }

    template<typename T>
    template<typename... Ds>
    inline tween<T> & tween<T>::during(Ds... ds) {
        total = 0;
        points.at(points.size() - 2).during(ds...);
        for (detail::tweenpoint<T> & p : points) {
            total += p.duration();
            p.stacked = total;
        }
        return *this;
    }

    template<typename T>
    inline const T & tween<T>::step(int32_t dt, bool suppress) {
        return step(static_cast<float>(dt * currentDirection)/static_cast<float>(total), suppress);
    }

    template<typename T>
    inline const T & tween<T>::step(uint32_t dt, bool suppress) {
        return step(static_cast<int32_t>(dt), suppress);
    }

    template<typename T>
    inline const T & tween<T>::step(float dp, bool suppress) {
        seek(currentProgress + dp, true);
        if (!suppress) dispatch(onStepCallbacks);
        return current;
    }

    template<typename T>
    inline const T & tween<T>::seek(float p, bool suppress) {
        p = detail::clip(p, 0.0f, 1.0f);
        currentProgress = p;
        render(p);
        if (!suppress) dispatch(onSeekCallbacks);
        return current;
    }

    template<typename T>
    inline const T & tween<T>::seek(int32_t t, bool suppress) {
        return seek(static_cast<float>(t) / static_cast<float>(total), suppress);
    }

    template<typename T>
    inline const T & tween<T>::seek(uint32_t t, bool suppress) {
        return seek(static_cast<float>(t) / static_cast<float>(total), suppress);
    }

    template<typename T>
    inline uint32_t tween<T>::duration() const {
        return total;
    }

    template<typename T>
    inline void tween<T>::interpolate(float prog, T & value) {
        auto & p = points.at(currentPoint);
        uint32_t pointDuration = p.duration() - (p.stacked - (prog * static_cast<float>(total)));
        float pointTotal = static_cast<float>(pointDuration) / static_cast<float>(p.duration());
        if (pointTotal > 1.0f) pointTotal = 1.0f;
        auto easing = std::get<0>(p.easings);
        value = easing(pointTotal, std::get<0>(p.values), std::get<0>(points.at(currentPoint+1).values));
    }

    template<typename T>
    inline void tween<T>::render(float p) {
        uint32_t t = static_cast<uint32_t>(p * static_cast<float>(total));
        if (t > points.at(currentPoint).stacked) currentPoint++;
        if (currentPoint > 0 && t <= points.at(currentPoint - 1u).stacked) currentPoint--;
        interpolate(p, current);
    }

    template<typename T>
    tween<T> & tween<T>::onStep(typename detail::tweentraits<T>::callbackType callback) {
        onStepCallbacks.push_back(callback);
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::onStep(typename detail::tweentraits<T>::noValuesCallbackType callback) {
        onStepCallbacks.push_back([callback](tween<T> & tween, T) { return callback(tween); });
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::onStep(typename detail::tweentraits<T>::noTweenCallbackType callback) {
        onStepCallbacks.push_back([callback](tween<T> &, T v) { return callback(v); });
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::onSeek(typename detail::tweentraits<T>::callbackType callback) {
        onSeekCallbacks.push_back(callback);
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::onSeek(typename detail::tweentraits<T>::noValuesCallbackType callback) {
        onSeekCallbacks.push_back([callback](tween<T> & t, T) { return callback(t); });
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::onSeek(typename detail::tweentraits<T>::noTweenCallbackType callback) {
        onSeekCallbacks.push_back([callback](tween<T> &, T v) { return callback(v); });
        return *this;
    }

    template<typename T>
    void tween<T>::dispatch(std::vector<typename traits::callbackType> & cbVector) {
        std::vector<size_t> dismissed;
        for (size_t i = 0; i < cbVector.size(); ++i) {
            auto && cb = cbVector[i];
            bool dismiss = cb(*this, current);
            if (dismiss) dismissed.push_back(i);
        }

        if (dismissed.size() > 0) {
            for (size_t i = 0; i < dismissed.size(); ++i) {
                size_t index = dismissed[i];
                cbVector[index] = cbVector.at(cbVector.size() - 1 - i);
            }
            cbVector.resize(cbVector.size() - dismissed.size());
        }
    }

    template<typename T>
    const T & tween<T>::peek() const {
        return current;
    }

    template<typename T>
    float tween<T>::progress() const {
        return currentProgress;
    }

    template<typename T>
    tween<T> & tween<T>::forward() {
        currentDirection = 1;
        return *this;
    }

    template<typename T>
    tween<T> & tween<T>::backward() {
        currentDirection = -1;
        return *this;
    }

    template<typename T>
    int tween<T>::direction() const {
        return currentDirection;
    }

    template<typename T>
    inline const T & tween<T>::jump(int32_t p, bool suppress) {
        p = detail::clip(p, 0, static_cast<int>(points.size() -1));
        return seek(points.at(p).stacked, suppress);
    }

    template<typename T> inline uint16_t tween<T>::point() const {
        return currentPoint;
    }
}
#endif //TWEENY_TWEENONE_TCC
