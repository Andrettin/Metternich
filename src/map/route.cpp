#include "metternich.h"

#include "map/route.h"

#include "economy/commodity.h"
#include "map/province.h"
#include "map/route_game_data.h"
#include "map/route_history.h"
#include "map/site.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

route::route(const std::string &identifier) : named_data_entry(identifier)
{
	this->reset_game_data();
}

route::~route()
{
}

void route::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "output_multiplier") {
		assert_throw(this->get_output_commodity() != nullptr);
		this->output_multiplier = this->get_output_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void route::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "path_provinces") {
		for (const std::string &value : values) {
			province *province = province::get(value);
			province->add_route(this);
			this->path_provinces.push_back(province);
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void route::initialize()
{
	if (this->get_start_site() != nullptr) {
		this->start_site->add_route(this);

		assert_throw(!this->get_path_provinces().empty());
		assert_throw(this->get_start_site()->get_province() == this->get_path_provinces().front());
	}

	if (this->get_end_site() != nullptr) {
		this->end_site->add_route(this);

		assert_throw(!this->get_path_provinces().empty());
		assert_throw(this->get_end_site()->get_province() == this->get_path_provinces().back());
	}

	named_data_entry::initialize();
}

void route::check() const
{
	if (this->get_output_commodity() == nullptr) {
		log::log_error(std::format("Route \"{}\" has no output commodity.", this->get_identifier()));
	} else if (this->get_start_site() == nullptr || this->get_end_site() == nullptr) {
		log::log_error(std::format("Route \"{}\" has an output commodity, but no start or end sites.", this->get_identifier()));
	}

	if (this->get_path_provinces().empty()) {
		log::log_error(std::format("Route \"{}\" has no path provinces.", this->get_identifier()));
	}
}

data_entry_history *route::get_history_base()
{
	return this->history.get();
}

void route::reset_history()
{
	this->history = make_qunique<route_history>(this);
}

void route::reset_game_data()
{
	this->game_data = make_qunique<route_game_data>(this);
}

}
