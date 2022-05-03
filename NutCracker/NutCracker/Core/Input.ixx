export import NutCracker.Core.KeyCodes;
export import NutCracker.Core.MouseCodes;

export import <tuple>;

export module NutCracker.Core.Input;

namespace NutCracker {
	export
	namespace Input
	{
		bool IsKeyPressed (KeyCode keycode);
		bool IsMouseButtonPressed (MouseCode button);

		std::pair<float, float> GetMousePosn ();
		float GetMouseX ();
		float GetMouseY ();
	};
}