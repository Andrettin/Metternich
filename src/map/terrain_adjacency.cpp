#include "metternich.h"

#include "map/terrain_adjacency.h"

#include "map/direction.h"
#include "map/terrain_adjacency_type.h"

namespace metternich {

std::string terrain_adjacency::to_string() const
{
	std::string str;

	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::northwest)));
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::north)));
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::northeast)));
	str += "\n";

	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::west)));
	str += std::to_string(static_cast<int>(terrain_adjacency_type::same));
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::east)));
	str += "\n";
	
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::southwest)));
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::south)));
	str += std::to_string(static_cast<int>(this->get_direction_adjacency_type(direction::southeast)));

	return str;
}

std::array<terrain_adjacency, 4> terrain_adjacency::get_subtile_adjacencies() const
{
	static constexpr terrain_adjacency solid_tile_adjacency;
	if (*this == solid_tile_adjacency) {
		return {};
	}

	std::array<terrain_adjacency, 4> subtile_adjacencies;

	if (this->get_direction_adjacency_type(direction::north) == terrain_adjacency_type::other) {
		subtile_adjacencies[0].set_direction_adjacency_type(direction::north, terrain_adjacency_type::other);
		subtile_adjacencies[1].set_direction_adjacency_type(direction::north, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::south) == terrain_adjacency_type::other) {
		subtile_adjacencies[2].set_direction_adjacency_type(direction::south, terrain_adjacency_type::other);
		subtile_adjacencies[3].set_direction_adjacency_type(direction::south, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::west) == terrain_adjacency_type::other) {
		subtile_adjacencies[0].set_direction_adjacency_type(direction::west, terrain_adjacency_type::other);
		subtile_adjacencies[2].set_direction_adjacency_type(direction::west, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::east) == terrain_adjacency_type::other) {
		subtile_adjacencies[1].set_direction_adjacency_type(direction::east, terrain_adjacency_type::other);
		subtile_adjacencies[3].set_direction_adjacency_type(direction::east, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::northwest) == terrain_adjacency_type::other) {
		subtile_adjacencies[0].set_direction_adjacency_type(direction::northwest, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::northeast) == terrain_adjacency_type::other) {
		subtile_adjacencies[1].set_direction_adjacency_type(direction::northeast, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::southwest) == terrain_adjacency_type::other) {
		subtile_adjacencies[2].set_direction_adjacency_type(direction::southwest, terrain_adjacency_type::other);
	}

	if (this->get_direction_adjacency_type(direction::southeast) == terrain_adjacency_type::other) {
		subtile_adjacencies[3].set_direction_adjacency_type(direction::southeast, terrain_adjacency_type::other);
	}

	return subtile_adjacencies;
}

}
