import NutCracker.Base.AppInstance;

#include <Core/Logging.hxx>;
#include <Utils/HelperMacros.hxx>;
import <string>;

import NutCracker.Base.Window;

export module NutCracker.Example.MultipleWindowed;

namespace NutCracker::Example {
	export
	class MultipleWindowed: public ::NutCracker::BaseInstance {
	public:
		void setup (const std::span<char*> argument_list) override { 
			LOG_trace(std::source_location::current ().function_name ());
			m_Window1 = Window::Create (WindowProps {
				.Title = "Vulkan Sandbox 1",
				.Width = 1280,
				.Height = 720,
			});
			m_Window2 = Window::Create (WindowProps {
				.Title = "Vulkan Sandbox 2",
				.Width = 1280,
				.Height = 720,
			});
			m_Window1->SetEventCallback (NTKR_BIND_FUNCTION (onEvent));
			m_Window2->SetEventCallback (NTKR_BIND_FUNCTION (onEvent));
		}

		void initializeVk () override 
			{ /* LOG_trace(std::source_location::current ().function_name ()); */ }

		void update (double update_latency) override 
			{ /* LOG_trace(std::source_location::current ().function_name ()); */ m_Window1->Update(); m_Window2->Update(); }

		void render (double render_latency) override 
			{ /* LOG_trace(std::source_location::current ().function_name ()); */ }

		void terminateVk () override 
			{ LOG_trace(std::source_location::current ().function_name ()); }

		void cleanup () override 
			{ LOG_trace(std::source_location::current ().function_name ()); delete m_Window1; delete m_Window2; }

		void onEvent (Event& e) override {LOG_trace ("{}", e);}
		bool keepContextRunning () override { static int counter = 10000; /* LOG_trace ("{:s}, counter {:d}", std::source_location::current ().function_name (), counter); */ return counter--; }

	private:
		bool m_Running = true;
		Window* m_Window1 = nullptr;
		Window* m_Window2 = nullptr;
	};
};