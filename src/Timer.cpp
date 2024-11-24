#include "Timer.hpp"

using namespace GalgameEngine;

Timer::Timer() noexcept
{
    __int64 countsPerSecond;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSecond);
    m_secondsPerCount = 1.0 / (double)countsPerSecond;
}

void Timer::reset() noexcept
{
    QueryPerformanceCounter((LARGE_INTEGER*)&m_baseTime);
    m_prevTime = m_baseTime;
    m_stopTime = 0;
    m_paused = false;
}

void Timer::resume() noexcept
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    if (m_paused)
    {
        m_paused = false;
        m_pausedTime += currTime - m_stopTime;
        m_stopTime = 0;
        m_prevTime = currTime;
    }
}

void Timer::pause() noexcept
{
    if (!m_paused)
    {
        m_paused = true;
        QueryPerformanceCounter((LARGE_INTEGER*)&m_stopTime);
    }
}

float Timer::getTime() const noexcept
{
    if (!m_paused)
    {
        return (float)(m_currTime - m_pausedTime - m_baseTime) * m_secondsPerCount;
    }
    else
    {
        return (float)(m_stopTime - m_pausedTime - m_baseTime) * m_secondsPerCount;
    }
}

void Timer::update() noexcept
{
    static double deltaTime = 0.0;
    if (m_paused)
    {
        deltaTime = 0;
        return;
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&m_currTime);
    
    deltaTime = (m_currTime - m_prevTime) * m_secondsPerCount;
    m_prevTime = m_currTime;

    if (deltaTime < 0.0)
    {
        deltaTime = 0.0;
    }
}

void Timer::calculateFrameState() noexcept
{
    static int   frameCnt    = 0;
    static float timeElapsed = 0.f;

    ++frameCnt;

    if (getTime() - timeElapsed >= 1.f)
    {
        m_fps  = (float)frameCnt;
        m_mspf = 1000.f / m_fps;

        frameCnt = 0;
        timeElapsed += 1.f;

        m_func(); 
    }
}