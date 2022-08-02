export import <string>;
import <ostream>;

#include <Utils/HelperMacros.hxx>

export module NutCracker.Base.Event;

namespace NutCracker {
	export 
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
	};
	export 
	enum EventCategory
	{
		None = 0,
		APPLICATION    = BIT (0),
		INPUT          = BIT (1),
		KEYBOARD       = BIT (2),
		MOUSE          = BIT (3),
		MOUSE_BUTTON   = BIT (4),
	};
	export 
	class Event
	{
	public:
		const uint8_t WindowNum;
		bool Handled = false;
		
		Event (const uint8_t win_num)
			: WindowNum (win_num) {}

		virtual constexpr EventType   GetEventType     () const = 0;
		virtual constexpr const char* GetName          () const = 0;
		virtual constexpr int         GetCategoryFlags () const = 0;
		virtual const std::string     ToString         () const { return GetName(); };

		virtual operator std::string();

		inline bool IsInCategory (EventCategory category)
		{
			return GetCategoryFlags () & category;
		}
	};
	export 
	class EventDispatcher
	{
	public:
		EventDispatcher (Event &event)
			: m_Event (event)
		{}
		
		// F will be deduced by the compiler,  F -> bool (*func) (T&)
		template<typename T, typename F>
		bool Dispatch (const F& func)
		{
			if (m_Event.GetEventType () == T::GetStaticType ())
			{
				m_Event.Handled = func (static_cast<T&> (m_Event));
				return true;
			}
			return false;
		}
		// F will be deduced by the compiler, F -> bool (*func) (Event&)
		template<EventCategory Flag, typename F>
		bool CategoryDispatch (const F& func)
		{
			if (m_Event.IsInCategory (Flag))
			{
				m_Event.Handled = func (m_Event);
				return true;
			}
			return false;
		}
	private:
		Event &m_Event;
	};
};

import <fmt/format.h>;

export 
inline std::ostream& operator << (std::ostream& os, const NutCracker::Event &e)
{
	return os << fmt::format ("from Wnd:{:d}, {:s}", e.WindowNum, e.ToString ());
}
NutCracker::Event::operator std::string() {
	return fmt::format ("from Wnd:{:d}, {:s}", WindowNum, ToString ());
}
export template<>
struct fmt::formatter<NutCracker::Event>: fmt::formatter<std::string>
{}; // Multiple hours wasted to finally arrive here (phew...)

//export template<>
//struct fmt::formatter<NutCracker::Event>
//{
//    constexpr auto parse (fmt::format_parse_context& ctx) -> decltype(ctx.end())
//    {
//        return ctx.end ();
//    }
//
//    template <typename FormatContext>
//    auto format (const NutCracker::Event& e, FormatContext& ctx) -> decltype(ctx.out())
//    {
//        return fmt::format_to (ctx.out (), "from Wnd:{}, {}", e.WindowNum, e.ToString ());
//    }
//};