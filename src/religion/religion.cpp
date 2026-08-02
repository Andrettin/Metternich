#include "metternich.h"

#include "religion/religion.h"

#include "religion/divine_domain.h"
#include "religion/religious_group.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"

namespace metternich {

void religion::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "divine_domains") {
		for (const std::string &value : values) {
			const divine_domain *domain = divine_domain::get(value);
			this->divine_domains.push_back(domain);
		}
	} else {
		religion_base::process_gsml_scope(scope);
	}
}

void religion::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Religion \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	if (this->get_group()->is_monotheistic()) {
		this->set_monotheistic(true);
	}

	named_data_entry::initialize();
}

void religion::check() const
{
	if (this->get_group() == nullptr) {
		throw std::runtime_error(std::format("Religion \"{}\" has no religious group.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Religion \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_deities().empty() && this->get_divine_domains().empty()) {
		log::log_error(std::format("Religion \"{}\" has no deities, and no divine domains of its own.", this->get_identifier()));
	}

	assert_throw(this->get_color().isValid());
}

const std::string &religion::get_title_name(const government_type *government_type, const domain_tier tier) const
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

const std::string &religion::get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender) const
{
	const std::string &office_title_name = religion_base::get_office_title_name(office, government_type, tier, gender);
	if (!office_title_name.empty()) {
		return office_title_name;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_office_title_name(office, government_type, tier, gender);
	}

	return string::empty_str;
}

}
