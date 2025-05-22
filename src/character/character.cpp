#include "metternich.h"

#include "character/character.h"

#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "character/character_role.h"
#include "character/character_type.h"
#include "character/dynasty.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "time/calendar.h"
#include "unit/civilian_unit_class.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/date_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"

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
	: character_base(identifier), role(character_role::none)
{
	this->reset_game_data();
}

character::~character()
{
}

void character::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "rulable_countries") {
		for (const std::string &value : values) {
			this->add_rulable_country(country::get(value));
		}
	} else if (tag == "traits") {
		for (const std::string &value : values) {
			this->traits.push_back(trait::get(value));
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "advisor_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->advisor_modifier = std::move(modifier);
	} else if (tag == "advisor_effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const country>>();
		database::process_gsml_data(effect_list, scope);
		this->advisor_effects = std::move(effect_list);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character::initialize()
{
	if (this->get_phenotype() == nullptr && this->get_culture() != nullptr) {
		this->phenotype = this->get_culture()->get_default_phenotype();
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

	if (this->get_character_type() != nullptr) {
		if (this->get_required_technology() == nullptr) {
			this->required_technology = this->get_character_type()->get_required_technology();

			if (this->get_obsolescence_technology() == nullptr) {
				this->obsolescence_technology = this->get_character_type()->get_obsolescence_technology();
			}
		}
	}

	if (this->get_role() != character_role::none) {
		if (this->required_technology != nullptr) {
			this->required_technology->add_enabled_character(this->get_role(), this);
		}

		if (this->obsolescence_technology != nullptr) {
			this->obsolescence_technology->add_retired_character(this->get_role(), this);
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
	if (this->get_role() == character_role::ruler || this->get_role() == character_role::advisor || this->get_role() == character_role::governor || this->get_role() == character_role::landholder) {
		if (this->get_character_type() == nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" is a ruler, advisor, governor or landholder, but has no character type.", this->get_identifier()));
		}
	}

	switch (this->get_role()) {
		case character_role::ruler: {
			if (this->get_rulable_countries().empty()) {
				throw std::runtime_error(std::format("Character \"{}\" is a ruler, but has no rulable countries.", this->get_identifier()));
			}

			if (this->get_character_type()->get_ruler_modifier() == nullptr && this->get_character_type()->get_scaled_ruler_modifier() == nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" is a ruler, but its character type (\"{}\") has no ruler modifier.", this->get_identifier(), this->get_character_type()->get_identifier()));
			}

			std::vector<const trait *> ruler_traits = this->get_traits();
			std::erase_if(ruler_traits, [](const trait *trait) {
				return !trait->get_types().contains(trait_type::ruler);
			});
			const int ruler_trait_count = static_cast<int>(ruler_traits.size());
			const int min_ruler_traits = defines::get()->get_min_traits_for_type(trait_type::ruler);
			const int max_ruler_traits = defines::get()->get_max_traits_for_type(trait_type::ruler);

			if (ruler_trait_count < min_ruler_traits) {
				log::log_error(std::format("Ruler character \"{}\" only has {} ruler {}, less than the expected minimum of {}.", this->get_identifier(), ruler_trait_count, ruler_trait_count == 1 ? "trait" : "traits", min_ruler_traits));
			} else if (ruler_trait_count > max_ruler_traits) {
				log::log_error(std::format("Ruler character \"{}\" has {} ruler {}, more than the expected maximum of {}.", this->get_identifier(), ruler_trait_count, ruler_trait_count == 1 ? "trait" : "traits", max_ruler_traits));
			}
			break;
		}
		case character_role::advisor:
			if (this->get_character_type()->get_advisor_modifier() == nullptr
				&& this->get_character_type()->get_scaled_advisor_modifier() == nullptr
				&& this->get_character_type()->get_advisor_effects() == nullptr
			) {
				throw std::runtime_error(std::format("Character \"{}\" is an advisor, but its character type (\"{}\") has no advisor modifier or effects.", this->get_identifier(), this->get_character_type()->get_identifier()));
			}
			break;
		case character_role::governor:
		{
			if (this->get_governable_province() == nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" is a governor, but has no governable province.", this->get_identifier()));
			}

			std::vector<const trait *> governor_traits = this->get_traits();
			std::erase_if(governor_traits, [](const trait *trait) {
				return trait->get_governor_modifier() == nullptr;
			});
			const int governor_trait_count = static_cast<int>(governor_traits.size());

			if (governor_trait_count == 0) {
				log::log_error(std::format("Governor character \"{}\" has no governor traits.", this->get_identifier()));
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

	if (this->get_role() != character_role::ruler && !this->get_rulable_countries().empty()) {
		throw std::runtime_error(std::format("Character \"{}\" has rulable countries, but is not a ruler.", this->get_identifier()));
	}

	if (this->get_role() != character_role::advisor) {
		if (this->advisor_modifier != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has an advisor modifier, but is not an advisor.", this->get_identifier()));
		}

		if (this->advisor_effects != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has advisor effects, but is not an advisor.", this->get_identifier()));
		}
	}

	if (this->get_role() != character_role::governor && this->get_governable_province() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has a governable province, but is not a governor.", this->get_identifier()));
	}

	if (this->get_role() != character_role::landholder && this->get_holdable_site() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has a holdable site, but is not a landholder.", this->get_identifier()));
	}

	assert_throw(this->get_culture() != nullptr);

	if (this->get_religion() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no religion.", this->get_identifier()));
	}

	assert_throw(this->get_phenotype() != nullptr);

	if (this->get_home_settlement() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no home settlement.", this->get_identifier()));
	} else if (!this->get_home_settlement()->is_settlement()) {
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

	const int adulthood_age = 15; //FIXME: make it variable according to character species
	if (adulthood_age == 0) {
		return false;
	}

	const int middle_age = 35; //FIXME: make it variable according to character species
	if (middle_age == 0) {
		return false;
	}

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

		const int parent_adulthood_age = 15; //FIXME: make it variable according to character species
		if (parent_adulthood_age == 0) {
			continue;
		}

		const int parent_middle_age = 35; //FIXME: make it variable according to character species
		if (parent_middle_age == 0) {
			continue;
		}

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
	if (this->get_character_type() != nullptr) {
		return this->get_character_type()->get_military_unit_category();
	}

	return military_unit_category::none;
}

const civilian_unit_class *character::get_civilian_unit_class() const
{
	if (this->get_character_type() != nullptr) {
		return this->get_character_type()->get_civilian_unit_class();
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

character_attribute character::get_primary_attribute() const
{
	if (this->get_character_type() != nullptr) {
		return this->get_character_type()->get_attribute();
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
	assert_throw(this->get_role() == character_role::leader);

	if (this->is_admiral()) {
		return "Admiral";
	} else if (this->is_explorer()) {
		return "Explorer";
	}

	return get_military_unit_category_name(this->get_military_unit_category());
}

}
