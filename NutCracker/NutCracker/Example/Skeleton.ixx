import NutCracker.Base.AppInstance;

#include <Core/Logging.hxx>;

export module NutCracker.Example.Skeleton;

namespace NutCracker::Example {
	export
	class Skeleton: public ::NutCracker::BaseInstance {
	protected:
		virtual void setup (const std::span<const char*> argument_list) override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void initializeVk () override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void update (double update_latency) override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void render (double render_latency) override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void terminateVk () override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void cleanup () override 
			{ LOG_trace (std::source_location::current ().function_name ()); }

		virtual void onEvent (NutCracker::Event&) override {}
		virtual bool keepContextRunning () override {static int counter = 10; LOG_trace ("{:s}, counter {:d}", std::source_location::current ().function_name (), counter); return counter--;}
	};
};