export import <string>;

export module NutCracker.Core.OSUtils;

namespace NutCracker {
	export 
	namespace FileDialogs
	{
		// returns Empty string if canceled
		std::string OpenFile (const char *filter);

		// returns Empty string if canceled
		std::string SaveFile (const char *filter);
	};
}