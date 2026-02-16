#include "metternich.h"

#include "character/bloodline.h"

#include "character/character.h"
#include "culture/culture.h"
#include "religion/deity.h"

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

void bloodline::initialize()
{
	if (this->get_founder() != nullptr) {
		this->founder->set_bloodline(this);

		if (this->get_founder()->get_deity() != nullptr) {
			this->founder->set_bloodline_strength(bloodline::max_bloodline_strength);
		} else {
			this->founder->set_bloodline_strength(this->founder->get_mythic_tier() * 10);
		}
	}

	named_data_entry::initialize();
}

void bloodline::check() const
{
	if (this->get_founder() == nullptr) {
		throw std::runtime_error(std::format("Bloodline \"{}\" has no founder.", this->get_identifier()));
	}

	if (this->get_founder()->get_deity() == nullptr && this->get_founder()->get_mythic_path() == nullptr) {
		throw std::runtime_error(std::format("Bloodline \"{}\" has a founder (\"{}\") who is neither a deity, nor a mythic character.", this->get_identifier(), this->get_founder()->get_identifier()));
	}
}

const std::string &bloodline::get_cultural_name(const culture *culture) const
{
	if (this->get_founder()->get_deity() != nullptr) {
		return this->get_founder()->get_deity()->get_cultural_name(culture);
	}

	return this->get_name();
}

void bloodline::set_founder(character *founder)
{
	if (founder == this->get_founder()) {
		return;
	}

	this->founder = founder;
}

}
