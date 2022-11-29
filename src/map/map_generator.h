#pragma once

#include "map/province_container.h"

namespace metternich {

class country;

class map_generator final
{
public:
	explicit map_generator(const QSize &size) : size(size)
	{
	}

	const QSize &get_size() const
	{
		return this->size;
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	void generate();

private:
	void generate_terrain();
	void generate_provinces();
	std::vector<QPoint> generate_province_seeds(const size_t seed_count);
	void expand_province_seeds(const std::vector<QPoint> &base_seeds);
	bool generate_country(const country *country);

private:
	QSize size = QSize(0, 0);
	int province_count = 0;
	std::vector<QPoint> province_seeds;
	std::vector<int> tile_provinces;
	std::map<int, std::set<int>> province_border_provinces;
	province_set generated_provinces;
	std::map<int, const province *> provinces_by_index;
	province_map<const country *> province_owners;
};

}
