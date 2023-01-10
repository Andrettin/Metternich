#include "metternich.h"

#include "character/character.h"

#include "character/character_game_data.h"
#include "character/character_type.h"
#include "country/culture.h"
#include "map/province.h"
#include "util/assert_util.h"
#include "util/gender.h"

namespace metternich {

character::character(const std::string &identifier) : named_data_entry(identifier), gender(gender::none)
{
	this->reset_game_data();
}

character::~character()
{
}

void character::initialize()
{
	if (this->get_home_province() != nullptr) {
		this->home_province->add_character(this);
	}

	if (this->get_phenotype() == nullptr && this->get_culture() != nullptr) {
		this->phenotype = this->get_culture()->get_default_phenotype();
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

	data_entry::initialize();
}

void character::check() const
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_home_province() != nullptr);

	if (this->get_gender() == gender::none) {
		throw std::runtime_error("Character \"" + this->get_identifier() + "\" has no gender.");
	}

	assert_throw(this->get_start_date().isValid());
	assert_throw(this->get_end_date().isValid());
	assert_throw(this->get_birth_date().isValid());
	assert_throw(this->get_death_date().isValid());

	assert_throw(this->get_start_date() >= this->get_birth_date());
	assert_throw(this->get_start_date() <= this->get_end_date());
	assert_throw(this->get_start_date() <= this->get_death_date());
	assert_throw(this->get_end_date() >= this->get_birth_date());
	assert_throw(this->get_end_date() <= this->get_death_date());
	assert_throw(this->get_birth_date() <= this->get_death_date());
}

void character::reset_game_data()
{
	this->game_data = make_qunique<character_game_data>(this);
}

std::string character::get_full_name() const
{
	const std::string &name = this->get_name();
	std::string full_name = name;

	if (full_name.empty() && !this->get_surname().empty()) {
		full_name = this->get_surname();
	}

	if (!this->get_surname().empty()) {
		if (!name.empty()) {
			full_name += " " + this->get_surname();
		}
	}

	return full_name;
}

const icon *character::get_portrait() const
{
	assert_throw(this->get_type() != nullptr);
	return this->get_type()->get_portrait();
}

}
