#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Key
{
    constexpr inline int W = GLFW_KEY_W;
    constexpr inline int A = GLFW_KEY_A;
    constexpr inline int S = GLFW_KEY_S;
    constexpr inline int D = GLFW_KEY_D;
    constexpr inline int Space = GLFW_KEY_SPACE;
}

class Event
{
public:
    enum Type
    {
        MouseScroll,
    };

    Event(Type type);

    void SetScrollOffset(double offset) { scrollOffset = offset; }
    double GetScrollOffset() const { return scrollOffset; }

    Type GetType() const { return type; }

private:
    Type type;
    double scrollOffset;
};

class EventListener
{
public:
    virtual void Listen(Event event) = 0;
};

class Window
{
public:
    static void Init(int width, int height);
    static void SetIcon(const std::string& filepath);
    static uint32_t GetWidth();
    static uint32_t GetHeight();
    static GLFWwindow* GetWindow();
    static std::vector<const char*> GetExtensions();
    static vk::UniqueSurfaceKHR CreateSurface(vk::Instance instance);

    static void Shutdown();
    static bool ShouldClose();
    static void PollEvents();
    static bool IsMinimized();

    static void Update();
    static bool MousePressed();
    static bool MouseRightPressed();
    static bool KeyPressed(int key);
    static glm::vec2 GetMousePos() { return currMousePos; }
    static glm::vec2 GetMouseMotion() { return currMousePos - lastMousePos; }

    void AddEventListener(EventListener* listener)
    {
        listeners.push_back(listener);
    }

    void OnScroll(double xoffset, double yoffset)
    {
        Event event{ Event::MouseScroll };
        event.SetScrollOffset(yoffset);

        for (auto& listener : listeners) {
            listener->Listen(event);
        }
    }

private:
    static inline GLFWwindow* window;
    static inline glm::vec2 currMousePos = { 0.0f, 0.0f };
    static inline glm::vec2 lastMousePos = { 0.0f, 0.0f };
    static inline std::vector<EventListener*> listeners;
};
