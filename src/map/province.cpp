#include "metternich.h"

#include "map/province.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

province::province(const std::string &identifier) : named_data_entry(identifier)
{
	this->reset_game_data();
}

province::~province()
{
}

void province::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "cultural_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const culture *culture = culture::get(property.get_key());
			this->cultural_names[culture] = property.get_value();
		});
	} else if (tag == "cultural_group_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const cultural_group *cultural_group = cultural_group::get(property.get_key());
			this->cultural_group_names[cultural_group] = property.get_value();
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void province::check() const
{
	if (this->get_capital_settlement() == nullptr) {
		log::log_error("Province \"" + this->get_identifier() + "\" has no capital settlement.");
	}

	assert_throw(this->get_color().isValid());
}

data_entry_history *province::get_history_base()
{
	return this->history.get();
}

void province::reset_history()
{
	this->history = make_qunique<province_history>(this);
}

void province::reset_game_data()
{
	this->game_data = make_qunique<province_game_data>(this);
}

const std::string &province::get_cultural_name(const culture *culture) const
{
	if (culture != nullptr) {
		const auto find_iterator = this->cultural_names.find(culture);
		if (find_iterator != this->cultural_names.end()) {
			return find_iterator->second;
		}

		const auto group_find_iterator = this->cultural_group_names.find(culture->get_group());
		if (group_find_iterator != this->cultural_group_names.end()) {
			return group_find_iterator->second;
		}
	}

	return this->get_name();
}

}
