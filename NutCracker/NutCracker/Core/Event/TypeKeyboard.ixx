export import NutCracker.Core.KeyCodes;
export import NutCracker.Base.Event;
import <cstdint>;

import <fmt/format.h>;

#define GET_NUTCRACKER_EVENTCLASS_HELPER
#include <Utils/HelperMacros.hxx>;

export module NutCracker.Core.Event.Keyboard;

namespace NutCracker {
	export
	class KeyEvent : public Event
	{
	public:
		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY (EventCategory::KEYBOARD | EventCategory::INPUT)
	protected:
		KeyEvent (const uint8_t win_num, const KeyCode keycode)
			: Event (win_num), m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

	export
	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent (const uint8_t win_num, const KeyCode keycode, const int repeatCount)
			: KeyEvent (win_num, keycode), m_RepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			return fmt::format ("KeyPressedEvent: {:d}[\'{}\'] {} repeats",  m_KeyCode, char (m_KeyCode), m_RepeatCount);
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};

	export
	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent (const uint8_t win_num, const KeyCode keycode)
			: KeyEvent (win_num, keycode) {}

		std::string ToString() const override
		{
			return fmt::format ("KeyReleasedEvent: {:d}[\'{}\']", m_KeyCode, char (m_KeyCode));
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	export
	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent (const uint8_t win_num, const KeyCode keycode)
			: KeyEvent (win_num, keycode) {}

		std::string ToString() const override
		{
			return fmt::format ("KeyTypedEvent: {:d}[\'{}\']", m_KeyCode, char (m_KeyCode));
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}