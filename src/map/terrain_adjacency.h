#pragma once

namespace archimedes {
	enum class direction;
}

namespace metternich {

enum class terrain_adjacency_type;

class terrain_adjacency final
{
public:
	static constexpr size_t direction_count = 8;

	void set_direction_adjacency_type(const direction direction, const terrain_adjacency_type adjacency_type)
	{
		this->direction_adjacency_types[static_cast<int>(direction)] = adjacency_type;
	}

	constexpr bool operator <(const terrain_adjacency &other) const
	{
		for (size_t i = 0; i < this->direction_adjacency_types.size(); ++i) {
			if (this->direction_adjacency_types[i] != other.direction_adjacency_types[i]) {
				return this->direction_adjacency_types[i] < other.direction_adjacency_types[i];
			}
		}

		return false;
	}

private:
	std::array<terrain_adjacency_type, terrain_adjacency::direction_count> direction_adjacency_types{};
};

}
