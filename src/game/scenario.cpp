#include "metternich.h"

#include "game/scenario.h"

#include "country/country.h"
#include "time/calendar.h"
#include "util/assert_util.h"
#include "util/container_util.h"

namespace metternich {
	
void scenario::initialize_all()
{
	data_type::initialize_all();

	scenario::sort_instances([](const scenario *a, const scenario *b) {
		if (a->get_start_date() != b->get_start_date()) {
			return a->get_start_date() < b->get_start_date();
		} else {
			return a->get_identifier() < b->get_identifier();
		}
	});
}

void scenario::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "default_countries") {
		for (const std::string &value : values) {
			this->default_countries.push_back(country::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void scenario::initialize()
{
	if (this->start_date_calendar != nullptr) {
		if (!this->start_date_calendar->is_initialized()) {
			this->start_date_calendar->initialize();
		}

		this->start_date = this->start_date.addYears(this->start_date_calendar->get_year_offset());
		this->start_date_calendar = nullptr;
	}

	named_data_entry::initialize();
}

void scenario::check() const
{
	assert_throw(this->get_start_date().isValid());
	assert_throw(this->get_map_template() != nullptr);
}

QVariantList scenario::get_default_countries_qvariant_list() const
{
	return container::to_qvariant_list(this->default_countries);
}

}
