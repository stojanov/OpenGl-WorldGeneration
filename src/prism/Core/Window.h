#pragma once

#include <GLFW/glfw3.h>

#include "SystemEventManager.h"
#include "WindowData.h"

namespace Prism::Core
{
	using UpdateCallback = std::function<void(float dt)>;
	using DrawCallback = std::function<void()>;
	
	class Window
	{
	public:
		Window();
		virtual ~Window();

		void Create(int w, int h, const char* name);
		SystemEventManager& GetInputManager() { return m_InputEventManager; }

		void BindWindow();
		void ProcessEvents();
		void SetEventCallback(EventCallback callback);

		int Window::GetWidth() const
		{
			return m_Data.Width;
		}

		int Window::GetHeight() const
		{
			return m_Data.Height;
		}
		
		void Close();
		GLFWwindow* GetNativeWindow() const { return m_WindowPtr;  }
		GLFWwindow* GetLoadingContext() const { return m_LoadingContext; }
	private:
		GLFWwindow* m_WindowPtr{ nullptr };
		GLFWwindow* m_LoadingContext{ nullptr }; // For multithreaded use
		SystemEventManager m_InputEventManager;
		WindowData m_Data;
	};
}
