#include "metternich.h"

#include "map/region.h"

#include "economy/resource.h"
#include "map/province.h"
#include "map/region_history.h"
#include "map/site.h"
#include "util/vector_util.h"

namespace metternich {

region::region(const std::string &identifier) : named_data_entry(identifier)
{
}

region::~region()
{
}

void region::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "resources") {
		scope.for_each_property([&](const gsml_property &property) {
			const resource *resource = resource::get(property.get_key());
			this->resource_counts[resource] = std::stoi(property.get_value());
		});
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void region::initialize()
{
	for (region *subregion : this->subregions) {
		//initialize subregions, so that the provinces of their own subregions are added to them
		if (!subregion->is_initialized()) {
			subregion->initialize();
		}

		if (subregion->get_world() == nullptr && this->get_world() != nullptr) {
			subregion->world = this->get_world();
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
