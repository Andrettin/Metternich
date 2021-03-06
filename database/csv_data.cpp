#include "database/csv_data.h"

#include <fstream>
#include <stdexcept>

namespace metternich {

csv_data csv_data::parse_file(const std::filesystem::path &filepath)
{
	if (!std::filesystem::exists(filepath)) {
		throw std::runtime_error("File \"" + filepath.string() + "\" not found.");
	}

	std::ifstream ifstream(filepath);

	if (!ifstream) {
		throw std::runtime_error("Failed to open file: " + filepath.string());
	}

	csv_data csv_data;

	std::string line;
	while (std::getline(ifstream, line)) {
		std::vector<std::string> line_values;

		size_t token_pos = 0;
		size_t delimiter_pos = 0;
		while ((delimiter_pos = line.find(csv_data::delimiter, token_pos)) != std::string::npos) {
			std::string token = line.substr(token_pos, delimiter_pos - token_pos);
			line_values.push_back(token);
			token_pos = delimiter_pos + 1;
		}
		std::string token = line.substr(token_pos, line.length() - token_pos);
		line_values.push_back(token);

		csv_data.values.push_back(line_values);
	}

	return csv_data;
}

}
