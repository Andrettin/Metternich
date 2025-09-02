#include "metternich.h"

#include "character/character.h"

#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "character/character_role.h"
#include "character/dynasty.h"
#include "character/starting_age_category.h"
#include "character/character_trait.h"
#include "character/character_trait_type.h"
#include "country/country.h"
#include "country/culture.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "religion/deity.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "technology/technology.h"
#include "time/calendar.h"
#include "unit/civilian_unit_class.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/date_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

const std::set<std::string> character::database_dependencies = {
	//characters must be initialized after provinces, as their initialization results in settlements being assigned to their provinces, which is necessary for getting the provinces for home sites
	province::class_identifier
};

void character::initialize_all()
{
	data_type::initialize_all();

	std::vector<character *> dateless_characters = character::get_all();
	std::erase_if(dateless_characters, [](const character *character) {
		return character->has_vital_dates();
	});

	bool changed = true;
	while (changed) {
		while (changed) {
			changed = false;

			for (character *character : dateless_characters) {
				if (character->has_vital_dates()) {
					continue;
				}

				const bool success = character->initialize_dates_from_children();
				if (success) {
					character->initialize_dates();
					changed = true;
				}
			}

			std::erase_if(dateless_characters, [](const character *character) {
				return character->has_vital_dates();
			});
		}

		changed = false;
		for (character *character : dateless_characters) {
			const bool success = character->initialize_dates_from_parents();
			if (success) {
				character->initialize_dates();
				changed = true;
			}
		}

		std::erase_if(dateless_characters, [](const character *character) {
			return character->has_vital_dates();
		});
	}
}

bool character::skill_compare(const character *lhs, const character *rhs)
{
	if (lhs->get_skill() != rhs->get_skill()) {
		return lhs->get_skill() > rhs->get_skill();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

character::character(const std::string &identifier)
	: character_base(identifier)
{
	this->reset_game_data();
}

character::~character()
{
}

void character::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "role") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->roles = { magic_enum::enum_cast<character_role>(value).value() };
	} else {
		character_base::process_gsml_property(property);
	}
}

void character::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "roles") {
		for (const std::string &value : values) {
			this->roles.insert(magic_enum::enum_cast<character_role>(value).value());
		}
	} else if (tag == "rulable_countries") {
		for (const std::string &value : values) {
			this->add_rulable_country(country::get(value));
		}
	} else if (tag == "traits") {
		for (const std::string &value : values) {
			this->traits.push_back(character_trait::get(value));
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character::initialize()
{
	if (this->get_deity() != nullptr) {
		if (this->get_name().empty()) {
			this->set_name(this->get_deity()->get_name());
		}

		if (this->get_name_word() == nullptr) {
			this->set_name_word(this->get_deity()->get_name_word());
		}
	}

	if (this->get_phenotype() == nullptr && this->get_culture() != nullptr) {
		this->phenotype = this->get_culture()->get_default_phenotype();
	}

	if (this->get_species() == nullptr && this->get_phenotype() == nullptr && this->get_culture() != nullptr && this->get_culture()->get_species().size() == 1) {
		this->species = const_cast<metternich::species *>(this->get_culture()->get_species().at(0));
	}

	if (this->get_species() == nullptr && this->get_phenotype() != nullptr) {
		this->species = this->get_phenotype()->get_species();
	} else if (this->get_species() != nullptr && this->get_phenotype() == nullptr && this->get_species()->get_phenotypes().size() == 1) {
		this->phenotype = this->get_species()->get_phenotypes().at(0);
	}

	if (this->get_surname().empty() && this->get_dynasty() != nullptr) {
		std::string surname;
		if (!this->get_dynasty()->get_prefix().empty()) {
			surname = this->get_dynasty()->get_prefix() + " ";
		}

		surname += this->get_dynasty()->get_name();
		this->set_surname(surname);
	}

	if (this->get_culture() != nullptr) {
		if (!this->get_culture()->is_initialized()) {
			this->culture->initialize();
		}

		if (this->has_name_variant()) {
			this->culture->add_personal_name(this->get_gender(), this->get_name_variant());
		}

		if (!this->get_surname().empty()) {
			this->culture->add_surname(this->get_gender(), this->get_surname());
		}

		if (this->get_phenotype() != nullptr) {
			this->culture->change_phenotype_weight(this->get_phenotype(), 1);
		}
	}

	if (this->home_site != nullptr) {
		if (this->home_site->is_settlement()) {
			if (this->get_home_settlement() != nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" has both a home settlement and a home site.", this->get_identifier()));
			}

			this->home_settlement = this->home_site;
		} else {
			if (this->home_site->get_province() == nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" has a home site (\"{}\") which has no province.", this->get_identifier(), this->home_site->get_identifier()));
			}

			this->home_settlement = this->home_site->get_province()->get_provincial_capital();
		}
	}

	if (this->get_character_class() != nullptr) {
		if (this->get_required_technology() == nullptr) {
			this->required_technology = this->get_character_class()->get_required_technology();

			if (this->get_obsolescence_technology() == nullptr) {
				this->obsolescence_technology = this->get_character_class()->get_obsolescence_technology();
			}
		}
	}

	for (const character_role role : this->get_roles()) {
		if (this->required_technology != nullptr) {
			this->required_technology->add_enabled_character(role, this);
		}

		if (this->obsolescence_technology != nullptr) {
			this->obsolescence_technology->add_retired_character(role, this);
		}
	}

	if (this->get_governable_province() != nullptr) {
		this->governable_province->add_governor(this);
	}

	if (this->get_holdable_site() != nullptr) {
		this->holdable_site->add_landholder(this);
	}

	character_base::initialize();
}

void character::check() const
{
	if (!this->get_roles().empty()) {
		if (this->get_character_class() == nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has a role, but has no character type.", this->get_identifier()));
		}

		if (this->get_roles().size() > 1 && this->get_roles() != std::set<character_role>{ character_role::advisor, character_role::civilian }) {
			throw std::runtime_error(std::format("Character \"{}\" has multiple roles, but those aren't advisor and civilian, the only combination allowed.", this->get_identifier()));
		}
	}

	for (const character_role role : this->get_roles()) {
		switch (role) {
			case character_role::ruler: {
				if (this->get_rulable_countries().empty()) {
					throw std::runtime_error(std::format("Character \"{}\" is a ruler, but has no rulable countries.", this->get_identifier()));
				}
				break;
			}
			case character_role::governor:
			{
				if (this->get_governable_province() == nullptr) {
					throw std::runtime_error(std::format("Character \"{}\" is a governor, but has no governable province.", this->get_identifier()));
				}
				break;
			}
			case character_role::landholder:
			{
				if (this->get_holdable_site() == nullptr) {
					throw std::runtime_error(std::format("Character \"{}\" is a landholder, but has no holdable site.", this->get_identifier()));
				}

				if (this->get_holdable_site()->get_type() == site_type::settlement) {
					throw std::runtime_error(std::format("The holdable site for character \"{}\" is a settlement.", this->get_identifier()));
				}

				if (this->get_holdable_site()->get_type() == site_type::habitable_world) {
					throw std::runtime_error(std::format("The holdable site for character \"{}\" is a habitable world.", this->get_identifier()));
				}
				break;
			}
			case character_role::leader:
				if (this->get_military_unit_category() == military_unit_category::none) {
					throw std::runtime_error(std::format("Character \"{}\" is a leader, but has no military unit category.", this->get_identifier()));
				}
				break;
			case character_role::civilian:
				if (this->get_civilian_unit_class() == nullptr) {
					throw std::runtime_error(std::format("Character \"{}\" is a civilian, but has no civilian unit class.", this->get_identifier()));
				}

				if (this->get_civilian_unit_type() == nullptr) {
					throw std::runtime_error(std::format("Character \"{}\" is a civilian, but its culture (\"{}\") has no civilian unit type for its civilian unit class (\"{}\").", this->get_identifier(), this->get_culture()->get_identifier(), this->get_civilian_unit_class()->get_identifier()));
				}
				break;
			default:
				break;
		}
	}

	if (!this->has_role(character_role::ruler) && !this->get_rulable_countries().empty()) {
		throw std::runtime_error(std::format("Character \"{}\" has rulable countries, but is not a ruler.", this->get_identifier()));
	}

	if (!this->has_role(character_role::governor) && this->get_governable_province() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has a governable province, but is not a governor.", this->get_identifier()));
	}

	if (!this->has_role(character_role::landholder) && this->get_holdable_site() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has a holdable site, but is not a landholder.", this->get_identifier()));
	}

	if (this->get_species() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no species.", this->get_identifier()));
	}

	if (this->get_culture() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no culture.", this->get_identifier()));
	}

	if (this->get_religion() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no religion.", this->get_identifier()));
	}

	if (this->get_phenotype() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no phenotype.", this->get_identifier()));
	}

	if (this->get_phenotype()->get_species() != this->get_species()) {
		throw std::runtime_error(std::format("Character \"{}\" has a species (\"{}\") which is different than its phenotype's species (\"{}\").", this->get_identifier(), this->get_species()->get_identifier(), this->get_phenotype()->get_species()->get_identifier()));
	}

	if (!vector::contains(this->get_culture()->get_species(), this->get_species())) {
		throw std::runtime_error(std::format("Character \"{}\" has a species (\"{}\") which is not allowed for its culture (\"{}\").", this->get_identifier(), this->get_species()->get_identifier(), this->get_culture()->get_identifier()));
	}

	if (this->get_home_settlement() == nullptr && !this->is_deity()) {
		throw std::runtime_error(std::format("Non-deity character \"{}\" has no home settlement.", this->get_identifier()));
	} else if (this->get_home_settlement() != nullptr && !this->get_home_settlement()->is_settlement()) {
		throw std::runtime_error(std::format("Character \"{}\" has \"{}\" set as their home settlement, but it is not a settlement site.", this->get_identifier(), this->get_home_settlement()->get_identifier()));
	}

	character_base::check();
}

data_entry_history *character::get_history_base()
{
	return this->history.get();
}

void character::reset_history()
{
	this->history = make_qunique<character_history>(this);
}

void character::reset_game_data()
{
	this->game_data = make_qunique<character_game_data>(this);
	emit game_data_changed();
}

bool character::initialize_dates_from_children()
{
	assert_throw(!this->has_vital_dates());

	if (this->get_children().empty()) {
		return false;
	}

	assert_throw(this->get_species() != nullptr);

	const int adulthood_age = this->get_species()->get_adulthood_age();
	assert_throw(adulthood_age != 0);

	const int middle_age = this->get_species()->get_middle_age();
	assert_throw(middle_age != 0);

	QDate earliest_child_birth_date;

	for (character_base *child_base : this->get_children()) {
		character *child = static_cast<character *>(child_base);
		if (!child->has_vital_dates()) {
			if (child->initialize_dates_from_children()) {
				child->initialize_dates();
			}
		}

		if (!child->get_birth_date().isValid()) {
			continue;
		}

		if (!earliest_child_birth_date.isValid() || child->get_birth_date() < earliest_child_birth_date) {
			earliest_child_birth_date = child->get_birth_date();
		}
	}

	if (!earliest_child_birth_date.isValid()) {
		return false;
	}

	QDate birth_date = earliest_child_birth_date;
	birth_date = birth_date.addYears(-random::get()->generate_in_range(adulthood_age, middle_age));
	this->set_birth_date(birth_date);
	log_trace(std::format("Set birth date for character \"{}\": {}.", this->get_identifier(), date::to_string(birth_date)));

	return true;
}

bool character::initialize_dates_from_parents()
{
	assert_throw(!this->has_vital_dates());

	std::vector<const character *> parents;
	if (this->get_father() != nullptr) {
		parents.push_back(this->get_father());
	}
	if (this->get_mother() != nullptr) {
		parents.push_back(this->get_mother());
	}

	if (parents.empty()) {
		return false;
	}

	int adulthood_age = 0;
	int middle_age = 0;

	QDate latest_parent_birth_date;

	for (const character *parent : parents) {
		if (!parent->get_birth_date().isValid()) {
			continue;
		}

		assert_throw(this->get_species() != nullptr);

		const int parent_adulthood_age = parent->get_species()->get_adulthood_age();
		assert_throw(parent_adulthood_age != 0);

		const int parent_middle_age = parent->get_species()->get_middle_age();
		assert_throw(parent_middle_age != 0);

		if (!latest_parent_birth_date.isValid() || parent->get_birth_date() > latest_parent_birth_date) {
			latest_parent_birth_date = parent->get_birth_date();
			adulthood_age = parent_adulthood_age;
			middle_age = parent_middle_age;
		}
	}

	if (!latest_parent_birth_date.isValid()) {
		return false;
	}

	QDate birth_date = latest_parent_birth_date;
	birth_date = birth_date.addYears(random::get()->generate_in_range(adulthood_age, middle_age));
	this->set_birth_date(birth_date);
	log_trace(std::format("Set birth date for character \"{}\": {}.", this->get_identifier(), date::to_string(birth_date)));

	return true;
}

bool character::is_surname_first() const
{
	if (this->get_culture() != nullptr) {
		return this->get_culture()->is_surname_first();
	}

	return false;
}

const military_unit_category character::get_military_unit_category() const
{
	if (this->get_character_class() != nullptr) {
		return this->get_character_class()->get_military_unit_category();
	}

	return military_unit_category::none;
}

const civilian_unit_class *character::get_civilian_unit_class() const
{
	if (this->get_character_class() != nullptr) {
		return this->get_character_class()->get_civilian_unit_class();
	}

	return nullptr;
}

const civilian_unit_type *character::get_civilian_unit_type() const
{
	const civilian_unit_class *civilian_unit_class = this->get_civilian_unit_class();
	if (civilian_unit_class != nullptr) {
		return this->get_culture()->get_civilian_class_unit_type(civilian_unit_class);
	}

	return nullptr;
}

int character::get_adulthood_age() const
{
	assert_throw(this->get_species() != nullptr);
	return this->get_species()->get_adulthood_age();
}

int character::get_venerable_age() const
{
	assert_throw(this->get_species() != nullptr);
	return this->get_species()->get_venerable_age();
}

const dice &character::get_maximum_age_modifier() const
{
	assert_throw(this->get_species() != nullptr);
	return this->get_species()->get_maximum_age_modifier();
}

const dice &character::get_starting_age_modifier() const
{
	assert_throw(this->get_species() != nullptr);
	return this->get_species()->get_starting_age_modifier(this->get_character_class() ? this->get_character_class()->get_starting_age_category() : starting_age_category::intuitive);
}

character_attribute character::get_primary_attribute() const
{
	if (this->get_character_class() != nullptr) {
		return this->get_character_class()->get_attribute();
	}

	return character_attribute::none;
}

centesimal_int character::get_skill_multiplier() const
{
	assert_throw(defines::get()->get_max_character_skill() > 0);
	return centesimal_int(this->get_skill()) / defines::get()->get_max_character_skill();
}

void character::set_skill_multiplier(const centesimal_int &skill_multiplier)
{
	assert_throw(defines::get()->get_max_character_skill() > 0);
	this->skill = (skill_multiplier * defines::get()->get_max_character_skill()).to_int();
}

void character::add_rulable_country(country *country)
{
	this->rulable_countries.push_back(country);
	country->add_ruler(this);
}

bool character::is_admiral() const
{
	return this->get_military_unit_category() == military_unit_category::heavy_warship;
}

bool character::is_explorer() const
{
	return this->get_military_unit_category() == military_unit_category::light_warship;
}

std::string_view character::get_leader_type_name() const
{
	assert_throw(this->has_role(character_role::leader));

	if (this->is_admiral()) {
		return "Admiral";
	} else if (this->is_explorer()) {
		return "Explorer";
	}

	return get_military_unit_category_name(this->get_military_unit_category());
}

}
