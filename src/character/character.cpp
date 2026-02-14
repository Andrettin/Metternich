#include "metternich.h"

#include "character/character.h"

#include "character/bloodline.h"
#include "character/bloodline_strength_category.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/character_history.h"
#include "character/dynasty.h"
#include "character/monster_type.h"
#include "character/starting_age_category.h"
#include "character/trait.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "game/game.h"
#include "item/item_type.h"
#include "language/name_generator.h"
#include "language/word.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "religion/deity.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "technology/technology.h"
#include "time/calendar.h"
#include "unit/civilian_unit_class.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/date_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/number_util.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"
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

	character::initialize_all_vital_dates();
	character::initialize_all_home_sites();
	character::initialize_all_bloodlines();
}

void character::initialize_all_vital_dates()
{
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

	for (character *character : character::get_all()) {
		if (!character->get_children().empty()) {
			character->sort_children();
		}
	}
}

void character::initialize_all_home_sites()
{
	std::vector<character *> siteless_characters = character::get_all();
	std::erase_if(siteless_characters, [](const character *character) {
		return character->get_home_site() != nullptr || character->is_innate_deity();
	});

	bool changed = true;
	while (changed) {
		while (changed) {
			changed = false;

			for (character *character : siteless_characters) {
				if (character->get_home_site() != nullptr) {
					continue;
				}

				const bool success = character->initialize_home_site_from_children();
				if (success) {
					changed = true;
				}
			}

			std::erase_if(siteless_characters, [](const character *character) {
				return character->get_home_site() != nullptr;
			});
		}

		changed = false;
		for (character *character : siteless_characters) {
			const bool success = character->initialize_home_site_from_parents();
			if (success) {
				changed = true;
			}
		}

		std::erase_if(siteless_characters, [](const character *character) {
			return character->get_home_site() != nullptr;
		});
	}
}

void character::initialize_all_bloodlines()
{
	std::vector<character *> bloodlineless_characters = character::get_all();
	std::erase_if(bloodlineless_characters, [](const character *character) {
		return character->get_bloodline() != nullptr;
	});

	for (character *character : bloodlineless_characters) {
		if (character->get_bloodline() != nullptr) {
			continue;
		}

		character->initialize_bloodline_from_parents();
	}
}

bool character::skill_compare(const character *lhs, const character *rhs)
{
	if (lhs->get_skill() != rhs->get_skill()) {
		return lhs->get_skill() > rhs->get_skill();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

character *character::generate(const metternich::species *species, const metternich::character_class *character_class, const int level, const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const std::vector<const trait *> &traits, const int hit_points, const std::vector<const item_type *> &items, const bool generate_bloodline, const bool temporary)
{
	assert_throw(species != nullptr);

	auto generated_character = make_qunique<character>(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString());
	generated_character->moveToThread(QApplication::instance()->thread());

	generated_character->temporary = temporary;

	generated_character->species = const_cast<metternich::species *>(species);
	generated_character->character_class = character_class;
	generated_character->level = level;
	generated_character->monster_type = monster_type;
	if (culture != nullptr) {
		generated_character->culture = const_cast<metternich::culture *>(culture);
	} else if (!generated_character->get_species()->get_cultures().empty()) {
		generated_character->culture = const_cast<metternich::culture *>(vector::get_random(generated_character->get_species()->get_cultures()));
	}
	generated_character->religion = religion;
	if (generated_character->get_culture() != nullptr && generated_character->get_culture()->get_default_phenotype() != nullptr && generated_character->get_culture()->get_default_phenotype()->get_species() == generated_character->get_species()) {
		generated_character->phenotype = generated_character->get_culture()->get_default_phenotype();
	} else if (generated_character->get_species()->get_phenotypes().size() == 1) {
		generated_character->phenotype = generated_character->get_species()->get_phenotypes().at(0);
	}
	generated_character->home_site = home_site;
	generated_character->set_start_date(game::get()->get_date());

	const archimedes::gender gender = random::get()->generate(2) == 0 ? gender::male : gender::female;
	generated_character->set_gender(gender);
	if (generated_character->get_culture() != nullptr) {
		const archimedes::name_generator *given_name_generator = generated_character->get_culture()->get_given_name_generator(gender);
		if (given_name_generator != nullptr) {
			generated_character->set_name(given_name_generator->generate_name());
			const archimedes::name_generator *surname_generator = generated_character->get_culture()->get_surname_generator(gender);
			if (surname_generator != nullptr) {
				generated_character->set_surname(surname_generator->generate_name());
			}
		}
	} else {
		const archimedes::name_generator *name_generator = generated_character->get_species()->get_given_name_generator(gender);
		if (name_generator != nullptr) {
			generated_character->set_name(name_generator->generate_name());
		}
	}

	if (hit_points != 0) {
		generated_character->hit_points = hit_points;
	}

	generated_character->starting_items = items;

	generated_character->initialize_dates();

	if (generate_bloodline) {
		generated_character->generated_bloodline = true;
		generated_character->initialize_bloodline();
	}

	generated_character->check();

	generated_character->reset_game_data();
	generated_character->get_game_data()->set_character_class(character_class);
	generated_character->get_game_data()->set_target_traits(traits);
	generated_character->get_game_data()->apply_species_and_class(level);
	generated_character->get_game_data()->on_setup_finished();

	game::get()->add_generated_character(std::move(generated_character));
	return game::get()->get_generated_characters().back().get();
}

character *character::generate(const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const int hit_points, const std::vector<const item_type *> &items, const bool generate_bloodline, const bool temporary)
{
	return character::generate(monster_type->get_species(), monster_type->get_character_class(), monster_type->get_level(), monster_type, culture, religion, home_site, monster_type->get_traits(), hit_points, items, generate_bloodline, temporary);
}

std::shared_ptr<character_reference> character::generate_temporary(const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const int hit_points, const std::vector<const item_type *> &items)
{
	metternich::character *character = character::generate(monster_type, culture, religion, home_site, hit_points, items, false, true);
	return std::make_shared<character_reference>(character);
}

character::character(const std::string &identifier)
	: character_base(identifier)
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

	if (tag == "attributes") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const int value_int = std::stoi(value);

			this->attribute_ranges[character_attribute::get(key)] = std::make_pair(value_int, value_int);
		});
	} else if (tag == "attribute_ratings") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->attribute_ratings[character_attribute::get(key)] = value;
		});
	} else if (tag == "traits") {
		for (const std::string &value : values) {
			this->traits.push_back(trait::get(value));
		}
	} else if (tag == "starting_items") {
		for (const std::string &value : values) {
			this->starting_items.push_back(item_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const item_type *item_type = item_type::get(key);
			const int quantity = std::stoi(value);

			for (int i = 0; i < quantity; ++i) {
				this->starting_items.push_back(item_type);
			}
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<domain>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "game_data") {
		scope.process(this->get_game_data());
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character::initialize()
{
	if (this->get_monster_type() != nullptr) {
		assert_throw(this->get_species() == nullptr);
		assert_throw(this->get_character_class() == nullptr);
		assert_throw(this->get_level() == 0);

		this->species = const_cast<metternich::species *>(this->get_monster_type()->get_species());
		this->character_class = this->get_monster_type()->get_character_class();
		this->level = this->get_monster_type()->get_level();
	}

	if (!this->rank.empty()) {
		assert_throw(this->get_level() == 0);
		assert_throw(this->get_character_class() != nullptr);
		this->level = this->get_character_class()->get_rank_level(this->rank);
		this->rank.clear();
	}

	for (const auto &[attribute, rating] : this->attribute_ratings) {
		this->attribute_ranges[attribute] = attribute->get_rating_range(rating);
	}

	if (this->get_deity() != nullptr) {
		if (this->get_name().empty()) {
			this->set_name(this->get_deity()->get_name());
		}

		if (this->get_name_word() == nullptr) {
			this->set_name_word(this->get_deity()->get_name_word());
		}
	}

	if (this->get_phenotype() == nullptr && this->get_culture() != nullptr && this->get_culture()->get_default_phenotype() != nullptr && (this->get_species() == nullptr || this->get_culture()->get_default_phenotype()->get_species() == this->get_species())) {
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
		this->set_surname(this->get_dynasty()->get_surname(this->get_gender()));
	}

	if (this->get_culture() != nullptr) {
		if (!this->get_culture()->is_initialized()) {
			this->culture->initialize();
		}

		if (this->has_name_variant()) {
			this->culture->add_given_name(this->get_gender(), this->get_name_variant());
		}

		if (!this->get_surname().empty()) {
			this->culture->add_surname(this->get_gender(), this->get_surname());
		}

		if (this->get_phenotype() != nullptr) {
			this->culture->change_phenotype_weight(this->get_phenotype(), 1);
		}
	} else if (this->get_species() != nullptr && !this->get_species()->is_sapient()) {
		if (!this->get_species()->is_initialized()) {
			this->species->initialize();
		}

		if (this->has_name_variant()) {
			this->species->add_given_name(this->get_gender(), this->get_name_variant());
		}
	}

	if (this->get_gender() == gender::none && this->get_species() != nullptr && !this->get_species()->is_sapient()) {
		this->set_gender(static_cast<archimedes::gender>(random::get()->generate_in_range(static_cast<int>(gender::none) + 1, static_cast<int>(gender::count) - 1)));
	}

	if (this->get_home_site() != nullptr && !this->get_home_site()->is_settlement() && this->get_home_site()->get_type() != site_type::dungeon) {
		if (this->get_home_site()->get_province() == nullptr) {
			throw std::runtime_error(std::format("Character \"{}\" has a home site (\"{}\") which has no province.", this->get_identifier(), this->get_home_site()->get_identifier()));
		}

		this->home_site = this->get_home_site()->get_province()->get_default_provincial_capital();
	}

	this->initialize_bloodline();

	character_base::initialize();
}

void character::check() const
{
	if (this->name_front_compound_element != nullptr) {
		assert_throw(this->get_culture() != nullptr);
		assert_throw(this->name_front_compound_element->get_language() == this->get_culture()->get_language());
	}

	if (this->name_rear_compound_element != nullptr) {
		assert_throw(this->get_culture() != nullptr);
		assert_throw(this->name_rear_compound_element->get_language() == this->get_culture()->get_language());
	}

	if (this->get_species() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no species.", this->get_identifier()));
	}

	if (this->get_character_class() == nullptr && this->get_monster_type() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has neither a character class nor a monster type.", this->get_identifier()));
	}

	if (this->get_culture() == nullptr && this->get_species()->is_sapient()) {
		throw std::runtime_error(std::format("Character \"{}\" has no culture.", this->get_identifier()));
	}

	if (this->get_religion() == nullptr && this->get_species()->is_sapient() && !this->is_temporary()) {
		throw std::runtime_error(std::format("Character \"{}\" has no religion.", this->get_identifier()));
	}

	if (this->get_phenotype() == nullptr) {
		throw std::runtime_error(std::format("Character \"{}\" has no phenotype.", this->get_identifier()));
	}

	if (this->get_phenotype()->get_species() != this->get_species()) {
		throw std::runtime_error(std::format("Character \"{}\" has a species (\"{}\") which is different than its phenotype's species (\"{}\").", this->get_identifier(), this->get_species()->get_identifier(), this->get_phenotype()->get_species()->get_identifier()));
	}

	if (this->get_culture() != nullptr && !vector::contains(this->get_culture()->get_species(), this->get_species())) {
		throw std::runtime_error(std::format("Character \"{}\" has a species (\"{}\") which is not allowed for its culture (\"{}\").", this->get_identifier(), this->get_species()->get_identifier(), this->get_culture()->get_identifier()));
	}

	if (this->get_character_class() != nullptr && !this->get_character_class()->is_allowed_for_species(this->get_species())) {
		throw std::runtime_error(std::format("Character \"{}\" has a species (\"{}\") which is not allowed for its character class (\"{}\").", this->get_identifier(), this->get_species()->get_identifier(), this->get_character_class()->get_identifier()));
	}

	if (this->get_home_site() == nullptr && !this->is_innate_deity() && !this->is_temporary()) {
		throw std::runtime_error(std::format("Character \"{}\" has no home site, but is neither an innate deity, nor temporary.", this->get_identifier()));
	} else if (this->get_home_site() != nullptr && !this->get_home_site()->is_settlement() && this->get_home_site()->get_type() != site_type::dungeon) {
		throw std::runtime_error(std::format("Character \"{}\" has \"{}\" set as their home site, but it is neither a holding, nor a habitable world, nor a dungeon.", this->get_identifier(), this->get_home_site()->get_identifier()));
	}

	if (this->get_bloodline() != nullptr && this->get_bloodline_strength() == 0) {
		throw std::runtime_error(std::format("Character \"{}\" has a bloodline, but no bloodline strength.", this->get_identifier()));
	}

	character_base::check();
}

data_entry_history *character::get_history_base()
{
	return this->history.get();
}

gsml_data character::to_gsml_data() const
{
	gsml_data data(this->get_identifier());

	data.add_property("name", this->get_name());
	if (!this->get_surname().empty()) {
		data.add_property("surname", this->get_surname());
	}
	if (this->get_character_class() != nullptr) {
		data.add_property("character_class", this->get_character_class()->get_identifier());
		data.add_property("level", std::to_string(this->get_level()));
	}
	if (this->get_monster_type() != nullptr) {
		data.add_property("monster_type", this->get_monster_type()->get_identifier());
	}
	if (this->get_culture() != nullptr) {
		data.add_property("culture", this->get_culture()->get_identifier());
	}
	if (this->get_religion() != nullptr) {
		data.add_property("religion", this->get_religion()->get_identifier());
	}
	if (this->get_phenotype() != nullptr) {
		data.add_property("phenotype", this->get_phenotype()->get_identifier());
	}
	if (this->get_home_site() != nullptr) {
		data.add_property("home_site", this->get_home_site()->get_identifier());
	}
	data.add_property("gender", std::string(magic_enum::enum_name(this->get_gender())));
	data.add_property("start_date", date::to_string(this->get_start_date()));
	data.add_property("birth_date", date::to_string(this->get_birth_date()));
	data.add_property("death_date", date::to_string(this->get_death_date()));

	gsml_data game_data = this->get_game_data()->to_gsml_data();
	game_data.set_tag("game_data");
	data.add_child(std::move(game_data));

	return data;
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

	if (this->get_biological_children().empty()) {
		return false;
	}

	assert_throw(this->get_species() != nullptr);

	const int adulthood_age = this->get_species()->get_adulthood_age();
	assert_throw(adulthood_age != 0);

	const int middle_age = this->get_species()->get_middle_age();
	assert_throw(middle_age != 0);

	QDate earliest_child_birth_date;

	for (character_base *child_base : this->get_biological_children()) {
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

	const std::vector<character *> parents = this->get_all_parents();

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

bool character::initialize_home_site_from_children()
{
	assert_throw(this->get_home_site() == nullptr);

	if (this->get_children().empty()) {
		return false;
	}

	std::vector<const site *> potential_sites;

	for (character_base *child_base : this->get_children()) {
		character *child = static_cast<character *>(child_base);
		if (child->get_home_site() == nullptr) {
			child->initialize_home_site_from_children();
		}

		if (child->get_home_site() == nullptr) {
			continue;
		}

		potential_sites.push_back(child->get_home_site());
	}

	if (potential_sites.empty()) {
		return false;
	}

	this->home_site = vector::get_random(potential_sites);
	log_trace(std::format("Set home site for character \"{}\": {}.", this->get_identifier(), this->get_home_site()->get_identifier()));

	return true;
}

bool character::initialize_home_site_from_parents()
{
	assert_throw(this->get_home_site() == nullptr);

	const std::vector<character *> parents = this->get_parents();

	if (parents.empty()) {
		return false;
	}

	std::vector<const site *> potential_sites;

	for (const character *parent : parents) {
		if (parent->get_home_site() == nullptr) {
			continue;
		}

		potential_sites.push_back(parent->get_home_site());
	}

	if (potential_sites.empty()) {
		return false;
	}

	this->home_site = vector::get_random(potential_sites);
	log_trace(std::format("Set home site for character \"{}\": {}.", this->get_identifier(), this->get_home_site()->get_identifier()));

	return true;
}

void character::initialize_bloodline()
{
	if (this->generated_bloodline) {
		this->generate_bloodline();
	}

	if (this->get_bloodline() != nullptr) {
		if (this->get_bloodline_strength() == 0) {
			if (this->bloodline_strength_category == bloodline_strength_category::none) {
				this->bloodline_strength_category = vector::get_random(defines::get()->get_weighted_bloodline_strength_categories());
			}

			assert_throw(this->bloodline_strength_category != bloodline_strength_category::none);

			this->bloodline_strength = random::get()->roll_dice(defines::get()->get_bloodline_strength_for_category(this->bloodline_strength_category));
		}

		this->bloodline_initialized = true;
	}
}

void character::initialize_bloodline_from_parents()
{
	assert_throw(this->get_bloodline() == nullptr);

	const std::vector<character *> parents = this->get_biological_parents();

	if (parents.empty()) {
		return;
	}

	std::vector<const metternich::bloodline *> potential_bloodlines;
	int best_bloodline_strength = 0;
	int bloodline_strength = 0;

	for (character *parent : parents) {
		if (parent->get_bloodline() == nullptr && !parent->bloodline_initialized) {
			parent->initialize_bloodline_from_parents();
		}

		if (parent->get_bloodline() == nullptr) {
			continue;
		}

		bloodline_strength += parent->get_bloodline_strength();

		if (parent->get_bloodline_strength() < best_bloodline_strength) {
			continue;
		}

		if (parent->get_bloodline_strength() > best_bloodline_strength) {
			potential_bloodlines.clear();
			best_bloodline_strength = parent->get_bloodline_strength();
		}

		potential_bloodlines.push_back(parent->get_bloodline());
	}

	if (potential_bloodlines.empty()) {
		return;
	}

	bloodline_strength /= 2;

	if (bloodline_strength > 0) {
		this->bloodline = vector::get_random(potential_bloodlines);
		this->bloodline_strength = bloodline_strength;
		log_trace(std::format("Set bloodline for character \"{}\": {} ({}).", this->get_identifier(), this->get_bloodline()->get_identifier(), bloodline_strength));
	}

	this->bloodline_initialized = true;
}

void character::set_name_front_compound_element(word *word)
{
	this->name_front_compound_element = word;
	word->set_name_front_compound_element(true);
}

void character::set_name_rear_compound_element(word *word)
{
	this->name_rear_compound_element = word;
	word->set_name_rear_compound_element(true);
}

std::string character::get_full_name(const metternich::domain *regnal_domain, const std::optional<int> &regnal_number) const
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
	} else if (regnal_number.has_value()) {
		assert_throw(regnal_domain != nullptr);

		if (!full_name.empty()) {
			full_name += " ";
		}

		full_name += number::to_roman_numeral(regnal_number.value()) + " of " + (regnal_domain->has_definite_article() ? "the " : "") + regnal_domain->get_name();
	} else if (!this->get_surname().empty()) {
		if (this->is_surname_first()) {
			if (!full_name.empty()) {
				full_name = this->get_surname() + " " + full_name;
			} else {
				full_name = this->get_surname();
			}
		} else {
			if (!full_name.empty()) {
				full_name += " ";
			}

			full_name += this->get_surname();
		}
	}

	return full_name;
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

bool character::is_innate_deity() const
{
	return this->get_deity() != nullptr && !this->get_deity()->is_apotheotic();
}

void character::generate_bloodline()
{
	assert_throw(this->get_bloodline() == nullptr);
	assert_throw(this->get_culture() != nullptr);

	std::vector<const metternich::bloodline *> potential_bloodlines;
	bool found_religion_compatible_bloodline = false;
	bool found_religious_group_compatible_bloodline = false;

	for (const metternich::bloodline *bloodline : bloodline::get_all()) {
		if (!bloodline->get_cultures().contains(this->get_culture())) {
			continue;
		}

		if (bloodline->get_founder() != nullptr && bloodline->get_founder()->get_start_date().isValid() && this->get_birth_date().isValid() && bloodline->get_founder()->get_start_date() > this->get_birth_date()) {
			continue;
		}

		const bool has_compatible_religion = bloodline->get_founder()->get_religion() == this->get_religion() || (bloodline->get_founder()->get_deity() != nullptr && vector::contains(bloodline->get_founder()->get_deity()->get_religions(), this->get_religion()));
		if (has_compatible_religion) {
			if (!found_religion_compatible_bloodline) {
				found_religion_compatible_bloodline = true;
				found_religious_group_compatible_bloodline = true;
				potential_bloodlines.clear();
			}
		} else {
			if (found_religion_compatible_bloodline) {
				continue;
			}

			bool has_compatible_religious_group = bloodline->get_founder()->get_religion()->get_group() == this->get_religion()->get_group();
			if (!has_compatible_religious_group && bloodline->get_founder()->get_deity() != nullptr) {
				for (const metternich::religion *religion : bloodline->get_founder()->get_deity()->get_religions()) {
					if (religion->get_group() == this->get_religion()->get_group()) {
						has_compatible_religious_group = true;
						break;
					}
				}
			}
			if (has_compatible_religious_group) {
				if (!found_religious_group_compatible_bloodline) {
					found_religious_group_compatible_bloodline = true;
					potential_bloodlines.clear();
				}
			} else {
				if (found_religious_group_compatible_bloodline) {
					continue;
				}
			}
		}

		for (int i = 0; i < bloodline->get_weight(); ++i) {
			potential_bloodlines.push_back(bloodline);
		}
	}

	if (!potential_bloodlines.empty()) {
		this->set_bloodline(vector::get_random(potential_bloodlines));
		log_trace(std::format("Generated bloodline for character \"{}\": {}.", this->get_identifier(), this->get_bloodline()->get_identifier()));
	}
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

bool character::is_immortal() const
{
	return this->is_innate_deity();
}

character *character::get_dynastic_parent() const
{
	if (this->get_mother() != nullptr) {
		if (this->get_father() == nullptr || (this->get_dynasty() != nullptr && this->get_mother()->get_dynasty() == this->get_dynasty() && this->get_father()->get_dynasty() != this->get_dynasty())) {
			return this->get_mother();
		}
	}

	return this->get_father();
}

const character_attribute *character::get_primary_attribute() const
{
	if (this->get_character_class() != nullptr) {
		return this->get_character_class()->get_attribute();
	}

	return nullptr;
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

std::optional<std::pair<int, int>> character::get_attribute_range(const character_attribute *attribute) const
{
	const auto find_iterator = this->attribute_ranges.find(attribute);
	if (find_iterator != this->attribute_ranges.end()) {
		return find_iterator->second;
	}

	if (this->get_monster_type() != nullptr) {
		return this->get_monster_type()->get_attribute_range(attribute);
	}

	return std::nullopt;
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
	assert_throw(this->get_military_unit_category() != military_unit_category::none);

	if (this->is_admiral()) {
		return "Admiral";
	} else if (this->is_explorer()) {
		return "Explorer";
	}

	return get_military_unit_category_name(this->get_military_unit_category());
}

named_data_entry *character::get_tree_parent() const
{
	character *dynastic_parent = this->get_dynastic_parent();

	if (dynastic_parent != nullptr && !dynastic_parent->is_hidden_in_tree()) {
		return dynastic_parent;
	}

	return nullptr;
}

QVariantList character::get_secondary_tree_parents() const
{
	QVariantList secondary_tree_parents;

	const named_data_entry *tree_parent = this->get_tree_parent();

	for (character *parent : this->get_parents()) {
		if (parent == tree_parent) {
			continue;
		}

		if (parent->is_hidden_in_tree()) {
			continue;
		}

		secondary_tree_parents.push_back(QVariant::fromValue(parent));
	}

	return secondary_tree_parents;
}

std::vector<const named_data_entry *> character::get_top_tree_elements() const
{
	const named_data_entry *top_tree_element = this;
	while (top_tree_element->get_tree_parent() != nullptr) {
		top_tree_element = top_tree_element->get_tree_parent();
	}
	assert_throw(top_tree_element != nullptr);

	return { top_tree_element };
}

bool character::is_hidden_in_tree() const
{
	if (!game::get()->is_loaded()) {
		return false;
	}

	return !this->get_game_data()->has_ever_existed();
}

QVariantList character::get_tree_characters() const
{
	std::vector<const named_data_entry *> tree_characters = this->get_top_tree_elements();

	for (size_t i = 0; i < tree_characters.size(); ++i) {
		for (const named_data_entry *child : tree_characters.at(i)->get_tree_children()) {
			if (child->is_hidden_in_tree()) {
				continue;
			}

			if (!vector::contains(tree_characters, child)) {
				tree_characters.push_back(child);
			}
		}
	}

	return container::to_qvariant_list(tree_characters);
}

}
