#pragma once
#include "Event.h"

namespace Prism
{
	template<EventType Type>
	class KeyEvent : public Event
	{
	public:
		PR_EVENT(Type)
		KeyEvent(Keyboard::Key key) : m_Key(key) {}
		Keyboard::Key GetKey() const { return m_Key; }
	private:
		Keyboard::Key m_Key;
	};

	using KeyDownEvent = KeyEvent<EventType::KeyDown>;
	using KeyPressedEvent = KeyEvent<EventType::KeyPressed>;
	using KeyReleasedEvent = KeyEvent<EventType::KeyReleased>;
}
