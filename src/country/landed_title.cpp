#include "metternich.h"

#include "country/landed_title.h"

#include "country/landed_title_tier.h"
#include "map/site.h"
#include "util/assert_util.h"

namespace metternich {

//must be initialized after sites, in their initialization function landed titles may be created
const std::set<std::string> landed_title::database_dependencies = { site::class_identifier };

landed_title::landed_title(const std::string &identifier)
	: named_data_entry(identifier), default_tier(landed_title_tier::none), min_tier(landed_title_tier::none), max_tier(landed_title_tier::none)
{
}

void landed_title::initialize()
{
	if (this->get_min_tier() == landed_title_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == landed_title_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	data_entry::initialize();
}

void landed_title::check() const
{
	assert_throw(this->get_country() != nullptr || this->get_site() != nullptr);
	assert_throw(this->get_default_tier() != landed_title_tier::none);
	assert_throw(this->get_min_tier() != landed_title_tier::none);
	assert_throw(this->get_max_tier() != landed_title_tier::none);
}

void landed_title::set_site(const metternich::site *site)
{
	this->site = site;
	this->default_tier = landed_title_tier::barony; //resource sites are baronies by default
}

}
