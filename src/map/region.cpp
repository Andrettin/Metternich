#include "metternich.h"

#include "map/region.h"

#include "map/province.h"
#include "map/region_history.h"
#include "map/site.h"
#include "util/vector_util.h"

namespace metternich {

const std::set<std::string> region::history_database_dependencies = {
	//must be loaded after provinces and sites, since it relies on their population data having been loaded first
	province::class_identifier,
	site::class_identifier
};

void region::load_history_database(const QDateTime &start_date, const timeline *current_timeline, const QObject *game_rules)
{
	data_type::load_history_database(start_date, current_timeline, game_rules);

	std::vector<region *> regions = region::get_all();

	std::sort(regions.begin(), regions.end(), [](const region *lhs, const region *rhs) {
		//give priority to smaller regions
		if (lhs->get_provinces().size() != rhs->get_provinces().size()) {
			return lhs->get_provinces().size() < rhs->get_provinces().size();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const region *region : regions) {
		region->get_history()->distribute_population();
	}
}

region::region(const std::string &identifier) : named_data_entry(identifier)
{
}

region::~region()
{
}

void region::initialize()
{
	for (region *subregion : this->subregions) {
		//initialize subregions, so that the sites of their own subregions are added to them
		if (!subregion->is_initialized()) {
			subregion->initialize();
		}

		//add provinces from subregions
		for (province *province : subregion->get_provinces()) {
			if (vector::contains(this->provinces, province)) {
				continue;
			}

			this->provinces.push_back(province);
			province->add_region(this);
		}
	}

	named_data_entry::initialize();
}

data_entry_history *region::get_history_base()
{
	return this->history.get();
}

void region::reset_history()
{
	this->history = make_qunique<region_history>(this);
}

void region::add_province(province *province)
{
	if (vector::contains(this->provinces, province)) {
		return;
	}

	this->provinces.push_back(province);
}

bool region::is_part_of(const region *other_region) const
{
	if (vector::contains(this->superregions, other_region)) {
		return true;
	}

	for (const region *superregion : this->superregions) {
		if (superregion->is_part_of(other_region)) {
			return true;
		}
	}

	return false;
}

}
