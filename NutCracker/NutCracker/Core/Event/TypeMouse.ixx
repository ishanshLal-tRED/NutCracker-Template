export import NutCracker.Core.MouseCodes;
export import NutCracker.Base.Event;
import <cstdint>;

import <fmt/format.h>;

#define GET_NUTCRACKER_EVENTCLASS_HELPER
#include <Utils/HelperMacros.hxx>;

export module NutCracker.Core.Event.Mouse;

namespace NutCracker {
	export
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent (const uint8_t win_num, const float x, const float y)
			: Event (win_num), m_MouseX (x), m_MouseY (y) {}

		inline float GetX () const { return m_MouseX; }
		inline float GetY () const { return m_MouseY; }

		const std::string ToString () const override
		{
			return fmt::format ("MouseMovedEvent: {:f}, {:f}", m_MouseX, m_MouseY);
		}	

		EVENT_CLASS_TYPE (MouseMoved)
		EVENT_CLASS_CATEGORY (EventCategory::MOUSE | EventCategory::INPUT)
	private:
		float m_MouseX, m_MouseY;
	};

	export
	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent (const uint8_t win_num, const float xOffset, const float yOffset)
			: Event (win_num), m_XOffset (xOffset), m_YOffset (yOffset) {}

		inline float    GetXOffset () const { return m_XOffset; }
		inline float    GetYOffset () const { return m_YOffset; }

		const std::string ToString () const override
		{
			return fmt::format ("MouseScrolledEvent: {:f}, {:f}", m_XOffset, m_YOffset);
		}

		EVENT_CLASS_TYPE (MouseScrolled)
		EVENT_CLASS_CATEGORY (EventCategory::MOUSE | EventCategory::INPUT)
	private:
		float m_XOffset, m_YOffset;
	};

	export
	class MouseButtonEvent : public Event
	{
	public:
		inline MouseCode GetMouseButton () const { return m_Button; }

		EVENT_CLASS_CATEGORY (EventCategory::MOUSE | EventCategory::INPUT | EventCategory::MOUSE_BUTTON)
	protected:
		MouseButtonEvent (const uint8_t win_num, const int button)
			: Event (win_num), m_Button (button) {}

		MouseCode m_Button;
	};

	export
	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent (const uint8_t win_num, const MouseCode button)
			: MouseButtonEvent (win_num, button) {}

		const std::string ToString () const override
		{
			return fmt::format ("MouseButtonPressedEvent: {:d}", m_Button);
		}

		EVENT_CLASS_TYPE (MouseButtonPressed)
	};

	export
	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent (const uint8_t win_num, const MouseCode button)
			: MouseButtonEvent (win_num, button) {}

		const std::string ToString () const override
		{
			return fmt::format ("MouseButtonReleasedEvent: {:d}", m_Button);
		}

		EVENT_CLASS_TYPE (MouseButtonReleased)
	};

}