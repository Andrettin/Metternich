#include "metternich.h"

#include "character/bloodline.h"

#include "character/character.h"
#include "culture/culture.h"

namespace metternich {

void bloodline::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "cultures") {
		for (const std::string &value : values) {
			this->cultures.insert(culture::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void bloodline::check() const
{
	if (this->get_founder() == nullptr) {
		throw std::runtime_error(std::format("Bloodline \"{}\" has no founder.", this->get_identifier()));
	}
}

void bloodline::set_founder(character *founder)
{
	if (founder == this->get_founder()) {
		return;
	}

	this->founder = founder;
	founder->set_bloodline(this);
	founder->set_bloodline_strength(bloodline::max_bloodline_strength);
}

}
