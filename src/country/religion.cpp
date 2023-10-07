#include "metternich.h"

#include "country/religion.h"

#include "country/religious_group.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"

namespace metternich {

void religion::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Religion \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	named_data_entry::initialize();
}

void religion::check() const
{
	if (this->get_group() == nullptr) {
		throw std::runtime_error("Religion \"" + this->get_identifier() + "\" has no religious group.");
	}

	assert_throw(this->get_color().isValid());
}

const std::string &religion::get_title_name(const government_type *government_type, const country_tier tier) const
{
	const std::string &title_name = religion_base::get_title_name(government_type, tier);
	if (!title_name.empty()) {
		return title_name;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_title_name(government_type, tier);
	}

	return string::empty_str;
}

const std::string &religion::get_ruler_title_name(const government_type *government_type, const country_tier tier, const gender gender) const
{
	const std::string &ruler_title_name = religion_base::get_ruler_title_name(government_type, tier, gender);
	if (!ruler_title_name.empty()) {
		return ruler_title_name;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_ruler_title_name(government_type, tier, gender);
	}

	return string::empty_str;
}

}
