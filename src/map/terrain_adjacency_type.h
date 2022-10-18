#pragma once

namespace metternich {

enum class terrain_adjacency_type {
	same,
	other,
	any
};

inline terrain_adjacency_type string_to_terrain_adjacency_type(const std::string &str)
{
	if (str == "same") {
		return terrain_adjacency_type::same;
	} else if (str == "other") {
		return terrain_adjacency_type::other;
	} else if (str == "any") {
		return terrain_adjacency_type::any;
	}

	throw std::runtime_error("Invalid terrain adjacency type: \"" + str + "\".");
}

}
