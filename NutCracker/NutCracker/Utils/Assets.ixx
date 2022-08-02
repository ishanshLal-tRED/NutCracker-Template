#include <Core/logging.hxx>;

import <filesystem>;
import <fstream>;
export import <string>;
export import <tuple>;
export import <vector>;

export module Nutcracker.Utils.Assets;


namespace NutCracker::Assets {
	export namespace Files {
		std::vector<char> ReadFromDisk (const std::string_view filename);
	}
	export namespace Images {
		// returns {image_data, width, height, channels}
		std::tuple<uint8_t*, uint32_t, uint32_t, uint32_t> ReadFromDisk (const std::string_view filename);

		void FreeImageMem (uint8_t* image_data);
		
	}
};

std::vector<char> NutCracker::Assets::Files::ReadFromDisk (const std::string_view filename) {
	std::ifstream file (filename.data (), std::ios::ate | std::ios::binary);
	if (!file.is_open ())
		THROW_critical ("unable to open file!");

	size_t file_size = size_t (file.tellg ());
	std::vector<char> buffer (file_size);
	file.seekg (0);
	
	file.read (buffer.data (), file_size);
	
	file.close ();
	return buffer;
}

#include "stb_image.h";

namespace NutCracker::Assets::Images {
	std::tuple<uint8_t*, uint32_t, uint32_t, uint32_t> ReadFromDisk (const std::string_view filename) {
		int width, height, channels;
	
		stbi_uc* image_data = stbi_load (filename.data (), &width, &height, &channels, STBI_rgb_alpha);
		if (image_data == nullptr)
			THROW_critical ("couldn't load image: \'{:s}\'", filename);
	
		return {image_data, width, height, channels};
	}
	void FreeImageMem (uint8_t* image_data) {
		stbi_image_free (image_data);
	}
}