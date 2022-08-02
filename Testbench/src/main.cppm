import NutCracker.Example.VulkanAPP;

#include <Nutcracker/Core/Logging.hxx>;

import <span>;
import <iostream>;

int main (int argc, const char* argv []) {
	using APP = NutCracker::Example::VulkanAPP;

	try {
		new APP;
		APP::Setup (std::span<const char*> (argv, argc));
		APP::InitializeVk ();
		APP::Run ();
	} catch (const std::exception& e) {
		LOG_error ("RUNTIME_ERROR \n\t{}", e.what());
		return EXIT_FAILURE;
	}
	APP::TerminateVk ();
	APP::Cleanup ();

	return EXIT_SUCCESS;
}
