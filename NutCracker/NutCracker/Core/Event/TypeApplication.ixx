export import NutCracker.Base.Event;
import <cstdint>;

import <fmt/format.h>;

#define GET_NUTCRACKER_EVENTCLASS_HELPER
#include <Utils/HelperMacros.hxx>;

export module NutCracker.Core.Event.Application;

namespace NutCracker {
	export
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent (const uint8_t win_num, uint32_t width, uint32_t height)
			: Event (win_num), m_Width (width), m_Height (height) {}

		inline uint32_t   GetWidth  () const { return m_Width; }
		inline uint32_t   GetHeight () const { return m_Height; }

		const std::string ToString  () const override
		{
			return fmt::format ("WindowResizeEvent: {:d}, {:d}", m_Width, m_Height);
		}

		EVENT_CLASS_TYPE (WindowResize)
		EVENT_CLASS_CATEGORY (EventCategory::APPLICATION)
	private:
		uint32_t m_Width, m_Height;
	};
	export
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE (WindowClose)
		EVENT_CLASS_CATEGORY (EventCategory::APPLICATION)
	};
	export
	class AppTickEvent : public Event
	{
	public:
		AppTickEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE (AppTick)
		EVENT_CLASS_CATEGORY (EventCategory::APPLICATION)
	};
	export
	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE (AppUpdate)
		EVENT_CLASS_CATEGORY (EventCategory::APPLICATION)
	};
	export
	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent (const uint8_t win_num)
			: Event (win_num) {}

		EVENT_CLASS_TYPE (AppRender)
		EVENT_CLASS_CATEGORY (EventCategory::APPLICATION)
	};
};