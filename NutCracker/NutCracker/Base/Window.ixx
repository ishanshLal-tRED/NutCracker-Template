export import <string>;
import <map>;
import <limits>;
import <functional>;

#include <Core/Logging.hxx>

export import NutCracker.Base.Event;

export module NutCracker.Base.Window;

// map is less memory intensive than unordered
template <> struct std::less<void*> {
    bool operator()(const void* a, const void* b) const { return uintptr_t (a) < uintptr_t (b); }
};
static std::map <void*, uint8_t, std::less<void*>> s_WindowToAssignedNum;

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
		using EventCallbackFn = std::function<void(Event&)>;
		//using EventCallbackFn = void(*)(Event&);
		static constexpr uint8_t InvalidWindow = std::numeric_limits<uint8_t>::max();

		virtual ~Window () = default;
		
		// 255 if window not initalized
		const uint8_t GetWindowNumber () {
			void* handle = GetNativeWindow ();
			if (auto itr = s_WindowToAssignedNum.find (handle); handle != nullptr && itr != s_WindowToAssignedNum.end ())
				return itr->second;
			else return InvalidWindow;
		}

		// Make window as current context
		virtual void SetAsTarget () const = 0;
		// Swap buffers and poll event
		virtual void Update() = 0;

		virtual const uint32_t GetWidth () const = 0;
		virtual const uint32_t GetHeight () const = 0;

		virtual const std::pair<uint32_t, uint32_t> GetFramebufferSize () const = 0;

		// Window attributes
		virtual void SetEventCallback (const EventCallbackFn& callback) = 0;
		virtual void SetVSync (const bool enabled) = 0;
		virtual const bool IsVSync () const = 0;

		virtual void MouseCursor (const bool show) const = 0;

		virtual void* GetNativeWindow () const = 0;

		static Window* Create (const WindowProps& props = WindowProps());

	protected:
		// only numeric_limits<uint8_t>::max() (aka 0-254) allowed
		static void AssignWindowNum (Window* in/* = this*/) {
			void* handle = in->GetNativeWindow ();
			if (s_WindowToAssignedNum.find (handle) != s_WindowToAssignedNum.end ())
				THROW_CORE_critical ("!!window already present, handle your windows with more caution !!");
			if (s_WindowToAssignedNum.size () == (InvalidWindow - 1))
				THROW_CORE_critical ("!!this is {:d}th window but our budget is only till {:d}, why do you need this much windows anyway !!", InvalidWindow, InvalidWindow - 1);
			
			if (s_WindowToAssignedNum.empty ()) {
				s_WindowToAssignedNum [handle] = 0;
				return;
			}

			if (uint8_t val = s_WindowToAssignedNum.rbegin ()->second; val < (InvalidWindow - 1))
				s_WindowToAssignedNum [handle] = val + 1;
			else {
				for (auto ritr = s_WindowToAssignedNum.rbegin (); ritr != s_WindowToAssignedNum.rend () && val == ritr->second; ritr++, val--);
				s_WindowToAssignedNum [handle] = val;
			}
		}

		static void UnAssignWindowNum (Window* in) {
			void* handle = in->GetNativeWindow ();
			if (auto itr = s_WindowToAssignedNum.find (handle); handle != nullptr && itr != s_WindowToAssignedNum.end ())
				s_WindowToAssignedNum.erase (itr);
		}
	};
};