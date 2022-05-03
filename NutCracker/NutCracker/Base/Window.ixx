export import <string>;
import <limits>;
import <map>;

#include <Core/Logging.hxx>

export import NutCracker.Base.Event;

export module NutCracker.Base.Window;

namespace NutCracker {
	export
	struct WindowProps
	{
		std::string Title = "Vulkan Sandbox";
		uint32_t Width = 1280;
		uint32_t Height = 720;
	};

	export // Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = void(*)(Event&);
		static constexpr uint8_t InvalidWindow = std::numeric_limits<uint8_t>::max();

		virtual ~Window() = default;
		
		// 255 if window not initalized
		const uint8_t GetWindowNumber () {
			if (auto itr = m_WindowToAssignedNum.find (GetNativeWindow ()); itr != m_WindowToAssignedNum.end ())
				return itr->second;
			else return InvalidWindow;
		}


		virtual bool Init (const WindowProps& props = WindowProps()) = 0;

		// Make window as current context
		virtual void SetAsTarget () = 0;
		// Swap buffers and poll event
		virtual void Update() = 0;

		virtual const uint32_t GetWidth () const = 0;
		virtual const uint32_t GetHeight () const = 0;

		// Window attributes
		virtual void SetEventCallback (const EventCallbackFn& callback) = 0;
		virtual void SetVSync (const bool enabled) = 0;
		virtual bool IsVSync () const = 0;

		virtual void MouseCursor (const bool show) const = 0;

		virtual void* GetNativeWindow () const = 0;
	private:
		// map is less memory intensive than unordered
		static std::map <void*, uint8_t, std::less<void*>> m_WindowToAssignedNum;

	protected:
		// only numeric_limits<uint8_t>::max() (aka 0-254) allowed
		static void AssignWindowNum (Window* in/* = this*/) {
			void* handle = in->GetNativeWindow ();
			if (m_WindowToAssignedNum.find (handle) != m_WindowToAssignedNum.end ())
				THROW_CORE_critical ("!!window already present, handle your windows with more caution !!");
			if (m_WindowToAssignedNum.size () == (InvalidWindow - 1))
				THROW_CORE_critical ("!!this is {:d}th window but our budget is only {:d}, why do you need this much !!", InvalidWindow, InvalidWindow - 1);

			if (uint8_t val = m_WindowToAssignedNum.rbegin ()->second; val < (InvalidWindow - 1))
				m_WindowToAssignedNum [handle] = val;
			else {
				for (auto ritr = m_WindowToAssignedNum.rbegin (); ritr != m_WindowToAssignedNum.rend () && val == ritr->second; ritr++, val--);
				m_WindowToAssignedNum [handle] = val;
			}
		}
	};
};