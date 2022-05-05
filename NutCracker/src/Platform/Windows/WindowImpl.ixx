import NutCracker.Base.Window;
import NutCracker.Events;

import <GLFW/glfw3.h>;

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>;

#include <Core/Logging.hxx>;

export module NutCracker.Windows.WindowImpl;

namespace NutCracker {
	export
	class WindowImpl: public Window {
	public:
		WindowImpl (const WindowProps&);
		virtual ~WindowImpl () override;

		// Make window as current context
		void SetAsTarget () const override final;
		// Swap buffers and poll event
		void Update() override final;

		inline const uint32_t GetWidth  () const override final { return m_Data.Width; }
		inline const uint32_t GetHeight () const override final { return m_Data.Height; }
		inline const std::pair<uint32_t, uint32_t> GetFramebufferSize () const {
			int width, height;
			glfwGetFramebufferSize (m_Handle, &width, &height);
			return {uint32_t(width), uint32_t(height)};
		}

		// Window attributes
		void SetEventCallback (const EventCallbackFn& callback) override final { m_Data.EventCallback = callback; }
		inline const bool IsVSync () const override final { return m_Data.VSync; }
		void SetVSync (const bool enabled) override final;

		void MouseCursor (const bool show) const override final;

		void* GetNativeWindow () const override final;
	private:
		static void GLFWErrorCallback(int error, const char* description)
		{
			LOG_error ("from GLFW, Code {0}: {1}", error, description);
		}

		GLFWwindow* m_Handle;

		std::string m_Title;
		struct struct_WindowData {
			uint32_t Width, Height;
			bool VSync;
			uint8_t WndNum = InvalidWindow;

			EventCallbackFn EventCallback;
		} m_Data;

		static uint8_t s_GLFWWindowCount;
	};
};

uint8_t NutCracker::WindowImpl::s_GLFWWindowCount = 0;
NutCracker::WindowImpl::WindowImpl (const WindowProps& props) {
	m_Title = props.Title;
	m_Data.Width = props.Width;
	m_Data.Height = props.Height;

	
	if (s_GLFWWindowCount == 0) {
		int success = glfwInit ();
		if (success == GLFW_FALSE)
			THROW_critical ("could not intialize GLFW");
		glfwSetErrorCallback (GLFWErrorCallback);
	}

	{// Create Window
	#if defined(NTKR_DEBUG)
		glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	#endif
		m_Handle = glfwCreateWindow ((int)props.Width, (int)props.Height, m_Title.c_str (), nullptr, nullptr);
		s_GLFWWindowCount++;

		AssignWindowNum (this);
		m_Data.WndNum = GetWindowNumber ();
	}

	glfwSetWindowUserPointer(m_Handle, &m_Data);
	SetVSync(true);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(m_Handle, [](GLFWwindow* window, int width, int height)
		{
			struct_WindowData& data = *(struct_WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(data.WndNum, width, height);
			data.EventCallback(event);
		});

	glfwSetWindowCloseCallback(m_Handle, [](GLFWwindow* window)
		{
			struct_WindowData& data = *(struct_WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event (data.WndNum);
			data.EventCallback(event);
		});

	glfwSetKeyCallback(m_Handle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			struct_WindowData& data = *(struct_WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(data.WndNum, key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(data.WndNum, key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(data.WndNum, key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

	glfwSetCharCallback(m_Handle, [](GLFWwindow* window, uint32_t keycode)
		{
			struct_WindowData& data = *(struct_WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(data.WndNum, keycode);
			data.EventCallback(event);
		});

	glfwSetMouseButtonCallback(m_Handle, [](GLFWwindow* window, int button, int action, int mods)
		{
			struct_WindowData& data = *(struct_WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(data.WndNum, button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(data.WndNum, button);
					data.EventCallback(event);
					break;
				}
			}
		});

	glfwSetScrollCallback(m_Handle, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto& data = *(struct_WindowData*) glfwGetWindowUserPointer(window);

			MouseScrolledEvent event(data.WndNum, (float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

	glfwSetCursorPosCallback(m_Handle, [](GLFWwindow* window, double xPos, double yPos)
		{
			auto& data = *(struct_WindowData*) glfwGetWindowUserPointer(window);

			MouseMovedEvent event(data.WndNum, (float)xPos, (float)yPos);
			data.EventCallback(event);
		});
}

NutCracker::WindowImpl::~WindowImpl () {
	glfwDestroyWindow(m_Handle);

	UnAssignWindowNum (this);
	m_Data.WndNum = Window::InvalidWindow;

	s_GLFWWindowCount--;
	if (s_GLFWWindowCount == 0)
		glfwTerminate ();
}

// Make window as current context
void NutCracker::WindowImpl::SetAsTarget () const {
	glfwMakeContextCurrent (m_Handle);
}

// Swap buffers and poll event
void NutCracker::WindowImpl::Update () {
	glfwSwapBuffers (m_Handle);
	glfwPollEvents ();
}

// Window attributes
void NutCracker::WindowImpl::SetVSync (const bool enabled) {
}

void NutCracker::WindowImpl::MouseCursor (const bool show) const {
	glfwSetInputMode (m_Handle, GLFW_CURSOR, show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void* NutCracker::WindowImpl::GetNativeWindow () const {
	return (void*)(glfwGetWin32Window (m_Handle)); // hwnd is essentially intptr_t
}