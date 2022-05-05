import NutCracker.Base;
import NutCracker.Events;
import NutCracker.Examples;

#include <NutCracker/Common.hxx>;

import <iostream>;
int main (int argc, char* argv []) {
	using APP = NutCracker::Example::VulkanExample;
	new APP;
	APP::InitializeVk ();
	APP::Setup (std::span<char*>(argv, argc));
	APP::Run ();
	APP::TerminateVk ();
	APP::Cleanup ();
	LOG_trace ("TestingStuff {}", NutCracker::Key::B);
}