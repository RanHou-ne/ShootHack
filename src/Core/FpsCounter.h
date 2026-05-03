#pragma once
#include <chrono>
#include <deque>

// ============================================================================
// FPS 计数器类
// 
// 功能：
//   - 计算实时帧率（使用滑动窗口平均）
//   - 支持多个独立的FPS计数器（游戏FPS、覆盖层FPS等）
//   - 提供平滑的FPS显示（避免数值跳动）
// 
// 使用方法：
//   FpsCounter gameFps;
//   
//   // 每帧调用
//   gameFps.Update();
//   
//   // 获取FPS
//   float fps = gameFps.GetFPS();
// ============================================================================

class FpsCounter
{
public:
    FpsCounter(size_t sampleCount = 60)
        : m_sampleCount(sampleCount)
        , m_fps(0.0f)
        , m_frameCount(0)
    {
        m_lastTime = std::chrono::high_resolution_clock::now();
    }

    // 每帧调用此函数更新FPS
    void Update()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - m_lastTime).count();
        m_lastTime = currentTime;

        // 添加到样本队列
        m_frameTimes.push_back(deltaTime);
        
        // 保持队列大小
        if (m_frameTimes.size() > m_sampleCount)
        {
            m_frameTimes.pop_front();
        }

        // 计算平均帧时间
        float totalTime = 0.0f;
        for (float time : m_frameTimes)
        {
            totalTime += time;
        }

        if (totalTime > 0.0f && !m_frameTimes.empty())
        {
            float avgFrameTime = totalTime / m_frameTimes.size();
            m_fps = 1.0f / avgFrameTime;
        }

        m_frameCount++;
    }

    // 获取当前FPS
    float GetFPS() const
    {
        return m_fps;
    }

    // 获取总帧数
    uint64_t GetFrameCount() const
    {
        return m_frameCount;
    }

    // 获取平均帧时间（毫秒）
    float GetAvgFrameTime() const
    {
        if (m_frameTimes.empty())
            return 0.0f;

        float totalTime = 0.0f;
        for (float time : m_frameTimes)
        {
            totalTime += time;
        }
        return (totalTime / m_frameTimes.size()) * 1000.0f; // 转换为毫秒
    }

    // 重置计数器
    void Reset()
    {
        m_frameTimes.clear();
        m_fps = 0.0f;
        m_frameCount = 0;
        m_lastTime = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::high_resolution_clock::time_point m_lastTime;
    std::deque<float> m_frameTimes;  // 帧时间样本队列
    size_t m_sampleCount;            // 样本数量（用于平滑）
    float m_fps;                     // 当前FPS
    uint64_t m_frameCount;           // 总帧数
};
