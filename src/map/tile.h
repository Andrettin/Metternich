#pragma once

#include "economy/commodity_container.h"
#include "util/centesimal_int.h"

namespace archimedes {
	enum class direction;
}

namespace metternich {

class domain;
class holding_type;
class province;
class resource;
class site;
class terrain_type;

class tile final
{
public:
	explicit tile(const terrain_type *terrain);

	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	void set_terrain(const terrain_type *terrain);

	metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(metternich::province *province)
	{
		this->province = province;
	}

	const domain *get_owner() const;

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site);

	const metternich::site *get_settlement() const;
	const holding_type *get_holding_type() const;

	const metternich::resource *get_resource() const;
	bool is_resource_discovered() const;

	const commodity_map<centesimal_int> &get_commodity_outputs() const;
	void calculate_commodity_outputs();

	bool produces_commodity(const commodity *commodity) const;

private:
	const terrain_type *terrain = nullptr;
	metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
};

}
