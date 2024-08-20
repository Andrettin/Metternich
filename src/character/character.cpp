#include "metternich.h"

#include "character/character.h"

#include "character/advisor_type.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "character/character_role.h"
#include "character/dynasty.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "time/calendar.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace metternich {

const std::set<std::string> character::database_dependencies = {
	//characters must be initialized after provinces, as their initialization results in settlements being assigned to their provinces, which is necessary for getting the provinces for home sites
	province::class_identifier
};

bool character::skill_compare(const character *lhs, const character *rhs)
{
	if (lhs->get_skill() != rhs->get_skill()) {
		return lhs->get_skill() > rhs->get_skill();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

character::character(const std::string &identifier) : named_data_entry(identifier), role(character_role::none), military_unit_category(military_unit_category::none), gender(gender::none)
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
		if (!this->get_dynasty()->get_prefix().empty()) {
			this->surname = this->get_dynasty()->get_prefix() + " ";
		}

		this->surname += this->get_dynasty()->get_name();
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

	if (this->vital_date_calendar != nullptr) {
		if (!this->vital_date_calendar->is_initialized()) {
			this->vital_date_calendar->initialize();
		}

		if (this->start_date.isValid()) {
			this->start_date = this->start_date.addYears(this->vital_date_calendar->get_year_offset());
		}

		if (this->end_date.isValid()) {
			this->end_date = this->end_date.addYears(this->vital_date_calendar->get_year_offset());
		}

		if (this->birth_date.isValid()) {
			this->birth_date = this->birth_date.addYears(this->vital_date_calendar->get_year_offset());
		}

		if (this->death_date.isValid()) {
			this->death_date = this->death_date.addYears(this->vital_date_calendar->get_year_offset());
		}

		this->vital_date_calendar = nullptr;
	}

	bool date_changed = true;
	while (date_changed) {
		date_changed = false;

		if (!this->get_start_date().isValid()) {
			if (this->get_birth_date().isValid()) {
				//if we have the birth date but not the start date, set the start date to when the character would become 30 years old
				this->start_date = this->get_birth_date().addYears(30);
				date_changed = true;
			}
		}

		if (!this->get_end_date().isValid() && this->get_death_date().isValid()) {
			this->end_date = this->get_death_date();
			date_changed = true;
		}

		if (!this->get_birth_date().isValid()) {
			if (this->get_start_date().isValid()) {
				this->birth_date = this->get_start_date().addYears(-30);
				date_changed = true;
			} else if (this->get_death_date().isValid()) {
				this->birth_date = this->get_death_date().addYears(-60);
				date_changed = true;
			}
		}

		if (!this->get_death_date().isValid()) {
			if (this->get_end_date().isValid()) {
				this->death_date = this->get_end_date();
				date_changed = true;
			} else if (this->get_birth_date().isValid()) {
				this->death_date = this->get_birth_date().addYears(60);
				date_changed = true;
			}
		}
	}

	if (this->get_advisor_type() != nullptr) {
		if (this->get_required_technology() == nullptr) {
			this->required_technology = this->get_advisor_type()->get_required_technology();

			if (this->get_obsolescence_technology() == nullptr) {
				this->obsolescence_technology = this->get_advisor_type()->get_obsolescence_technology();
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

	named_data_entry::initialize();
}

void character::check() const
{
	switch (this->get_role()) {
		case character_role::ruler:
			if (this->get_rulable_countries().empty()) {
				throw std::runtime_error(std::format("Character \"{}\" is a ruler, but has no rulable countries.", this->get_identifier()));
			}

			if (this->get_traits().size() < character::ruler_trait_count) {
				log::log_error(std::format("Character \"{}\" is a ruler, but only has {} {}, instead of the expected {}.", this->get_identifier(), this->get_traits().size(), this->get_traits().size() == 1 ? "trait" : "traits", character::ruler_trait_count));
			}
			break;
		case character_role::advisor:
			if (this->get_advisor_type() == nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" is an advisor, but has no advisor type.", this->get_identifier()));
			}
			break;
		case character_role::leader:
			if (this->get_military_unit_category() == military_unit_category::none) {
				throw std::runtime_error(std::format("Character \"{}\" is a leader, but has no military unit category.", this->get_identifier()));
			}
			break;
		case character_role::civilian:
			if (this->get_civilian_unit_type() == nullptr) {
				throw std::runtime_error(std::format("Character \"{}\" is a civilian, but has no civilian unit type.", this->get_identifier()));
			}
			break;
		default:
			break;
	}

	if (this->get_role() != character_role::ruler && !this->get_rulable_countries().empty()) {
		throw std::runtime_error(std::format("Character \"{}\" has rulable countries, but is not a ruler.", this->get_identifier()));
	}

	if (this->get_role() != character_role::advisor) {
		if (this->get_advisor_type() != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has an advisor type, but is not an advisor.", this->get_identifier()));
		}

		if (this->advisor_modifier != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has an advisor modifier, but is not an advisor.", this->get_identifier()));
		}

		if (this->advisor_effects != nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has advisor effects, but is not an advisor.", this->get_identifier()));
		}
	}

	if (this->get_role() != character_role::leader && this->get_military_unit_category() != military_unit_category::none) {
		throw std::runtime_error(std::format("Character \"{}\" has a military unit category, but is not a leader.", this->get_identifier()));
	}

	if (this->get_role() != character_role::civilian && this->get_civilian_unit_type() != nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has a civilian unit type, but is not a civilian.", this->get_identifier()));
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

	if (this->get_gender() == gender::none) {
		throw std::runtime_error(std::format("Character \"{}\" has no gender.", this->get_identifier()));
	}

	if (this->get_father() != nullptr && this->get_father()->get_gender() != gender::male) {
		throw std::runtime_error("The father of character \"" + this->get_identifier() + "\" is not male.");
	}

	if (this->get_mother() != nullptr && this->get_mother()->get_gender() != gender::female) {
		throw std::runtime_error("The mother of character \"" + this->get_identifier() + "\" is not female.");
	}

	assert_throw(this->get_start_date().isValid());
	assert_throw(this->get_end_date().isValid());

	if (!this->get_birth_date().isValid()) {
		throw std::runtime_error(std::format("Character \"{}\" has no birth date.", this->get_identifier()));
	}

	if (!this->get_death_date().isValid()) {
		throw std::runtime_error(std::format("Character \"{}\" has no birth date.", this->get_identifier()));
	}

	assert_throw(this->get_start_date() >= this->get_birth_date());
	assert_throw(this->get_start_date() <= this->get_end_date());
	assert_throw(this->get_start_date() <= this->get_death_date());
	assert_throw(this->get_end_date() >= this->get_birth_date());
	assert_throw(this->get_end_date() <= this->get_death_date());
	assert_throw(this->get_birth_date() <= this->get_death_date());
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

std::string character::get_full_name() const
{
	if (!this->get_nickname().empty()) {
		return this->get_nickname();
	}

	const std::string &name = this->get_name();
	std::string full_name = name;

	if (!this->get_epithet().empty()) {
		if (!full_name.empty()) {
			full_name += " ";
		}

		full_name += this->get_epithet();
	} else if (!this->get_surname().empty()) {
		if (!full_name.empty()) {
			full_name += " ";
		}

		full_name += this->get_surname();
	}

	return full_name;
}

void character::add_rulable_country(country *country)
{
	this->rulable_countries.push_back(country);
	country->add_ruler(this);
}

std::string character::get_ruler_modifier_string(const country *country) const
{
	assert_throw(this->get_role() == character_role::ruler);

	std::string str;

	for (const trait *trait : this->get_traits()) {
		if (trait->get_ruler_modifier() == nullptr) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		str += string::highlight(trait->get_name());
		str += "\n" + trait->get_ruler_modifier()->get_string(country, 1, 1);
	}

	return str;
}

QString character::get_advisor_effects_string(metternich::country *country) const
{
	assert_throw(this->get_role() == character_role::advisor);

	std::string str;

	if (this->advisor_modifier != nullptr) {
		str += this->advisor_modifier->get_string(country);
	}

	if (this->advisor_effects != nullptr) {
		if (!str.empty()) {
			str += '\n';
		}

		str += this->advisor_effects->get_effects_string(country, read_only_context(country));
	}

	if (!str.empty()) {
		return QString::fromStdString(str);
	}

	if (this->get_advisor_type()->get_modifier() != nullptr) {
		str = this->get_advisor_type()->get_modifier()->get_string(country);
	} else if (this->get_advisor_type()->get_scaled_modifier() != nullptr) {
		str = this->get_advisor_type()->get_scaled_modifier()->get_string(country, this->get_skill());
	}
	
	if (this->get_advisor_type()->get_effects() != nullptr) {
		if (!str.empty()) {
			str += '\n';
		}

		str += this->get_advisor_type()->get_effects()->get_effects_string(country, read_only_context(country));
	}

	if (this->get_game_data()->get_country() != country) {
		const character *replaced_advisor = country->get_game_data()->get_replaced_advisor_for(this);
		if (replaced_advisor != nullptr) {
			if (!str.empty()) {
				str += '\n';
			}

			str += std::format("Replaces {}", replaced_advisor->get_full_name());
		}
	}

	return QString::fromStdString(str);
}

void character::apply_advisor_modifier(const country *country, const int multiplier) const
{
	assert_throw(this->get_role() == character_role::advisor);

	if (this->advisor_effects != nullptr) {
		return;
	}

	if (this->advisor_modifier != nullptr) {
		this->advisor_modifier->apply(country, multiplier);
	} else if (this->get_advisor_type()->get_modifier() != nullptr) {
		this->get_advisor_type()->get_modifier()->apply(country);
	} else if (this->get_advisor_type()->get_scaled_modifier() != nullptr) {
		this->get_advisor_type()->get_scaled_modifier()->apply(country, this->get_skill() * multiplier);
	}
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
