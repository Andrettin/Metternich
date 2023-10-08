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

	constexpr terrain_adjacency()
	{
	}

	terrain_adjacency(std::map<direction, terrain_adjacency_type> &&data)
	{
		for (const auto &[direction, adjacency_type] : data) {
			this->set_direction_adjacency_type(direction, adjacency_type);
		}
	}

	terrain_adjacency_type get_direction_adjacency_type(const direction direction) const
	{
		return this->direction_adjacency_types[static_cast<int>(direction)];
	}

	void set_direction_adjacency_type(const direction direction, const terrain_adjacency_type adjacency_type)
	{
		this->direction_adjacency_types[static_cast<int>(direction)] = adjacency_type;
	}

	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &get_data() const
	{
		return this->direction_adjacency_types;
	}

	std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &get_data()
	{
		return this->direction_adjacency_types;
	}

	std::string to_string() const;

	std::array<terrain_adjacency, 4> get_subtile_adjacencies() const;

	constexpr bool operator ==(const terrain_adjacency &other) const
	{
		return this->direction_adjacency_types == other.direction_adjacency_types;
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
