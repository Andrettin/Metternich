#include "metternich.h"

#include "character/monster_type.h"

#include "character/character_attribute.h"
#include "character/character_class.h"
#include "script/modifier.h"
#include "species/species.h"

namespace metternich {

monster_type::monster_type(const std::string &identifier)
	: named_data_entry(identifier)
{
}

monster_type::~monster_type()
{
}

void monster_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "attribute_ratings") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->attribute_ratings[character_attribute::get(key)] = value;
		});
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void monster_type::initialize()
{
	for (const auto &[attribute, rating] : this->attribute_ratings) {
		this->attribute_ranges[attribute] = attribute->get_rating_range(rating);
	}

	named_data_entry::initialize();
}

void monster_type::check() const
{
	if (this->get_species() == nullptr) {
		throw std::runtime_error(std::format("Monster type \"{}\" has no species.", this->get_identifier()));
	}

	if (this->get_character_class() != nullptr && this->get_level() == 0) {
		throw std::runtime_error(std::format("Monster type \"{}\" has a character class, but no level.", this->get_identifier()));
	}

	if (this->get_character_class() == nullptr) {
		if (this->get_damage_dice().is_null()) {
			throw std::runtime_error(std::format("Monster type \"{}\" has null damage dice, and no character class.", this->get_identifier()));
		}
	}
}

}
