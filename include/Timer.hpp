#pragma once

#include <functional>

namespace GalgameEngine
{
    class Timer
    {
    public:
        Timer() noexcept;
        ~Timer() = default;

        Timer(const Timer&)            = delete;
        Timer(Timer&&)                 = delete;
        Timer& operator=(const Timer&) = delete;
        Timer& operator=(Timer&&)      = delete;

        void reset()  noexcept;
        void resume() noexcept;
        void pause()  noexcept;

        void update() noexcept;

        void setFunc(const std::function<void()>& func) { m_func = func; }
        void calculateFrameState() noexcept;

        float getTime() const noexcept;
        float getFPS()  const noexcept { return m_fps; }
        float getMSPF() const noexcept { return m_mspf; }
        float getDelta() const noexcept { return m_delta; }

    private:
        double m_secondsPerCount;

        __int64 m_baseTime   = 0;
        __int64 m_pausedTime = 0;
        __int64 m_stopTime   = 0;
        __int64 m_prevTime   = 0;
        __int64 m_currTime   = 0;

        float     m_fps      = 0.f;
        float     m_mspf     = 0.f;
        bool      m_paused   = false;
        float     m_delta    = 0.f;

        std::function<void()> m_func = []{};
    };
}