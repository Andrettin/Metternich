#include "metternich.h"

#include "population/population_unit.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/ideology.h"
#include "country/religion.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "script/condition/condition.h"
#include "script/factor.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"

namespace metternich {

population_unit::population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const site *settlement)
	: type(type), culture(culture), religion(religion), phenotype(phenotype), country(settlement->get_game_data()->get_owner()), settlement(settlement)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_country() != nullptr);

	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

const icon *population_unit::get_icon() const
{
	return this->get_type()->get_phenotype_icon(this->get_phenotype());
}

void population_unit::set_type(const population_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	this->get_country()->get_game_data()->change_population_type_count(this->get_type(), -1);

	this->type = type;

	this->get_country()->get_game_data()->change_population_type_count(this->get_type(), 1);

	emit type_changed();
}

void population_unit::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->get_country()->get_game_data()->change_population_culture_count(this->get_culture(), -1);

	this->culture = culture;

	this->get_country()->get_game_data()->change_population_culture_count(this->get_culture(), 1);

	const population_type *culture_population_type = culture->get_population_class_type(this->get_type()->get_population_class());
	if (culture_population_type != this->get_type()) {
		this->set_type(culture_population_type);
	}

	emit culture_changed();
}

void population_unit::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->get_country()->get_game_data()->change_population_religion_count(this->get_religion(), -1);

	this->religion = religion;

	this->get_country()->get_game_data()->change_population_religion_count(this->get_religion(), 1);

	emit religion_changed();
}

void population_unit::set_phenotype(const metternich::phenotype *phenotype)
{
	if (phenotype == this->get_phenotype()) {
		return;
	}

	this->get_country()->get_game_data()->change_population_phenotype_count(this->get_phenotype(), -1);

	this->phenotype = phenotype;

	this->get_country()->get_game_data()->change_population_phenotype_count(this->get_phenotype(), 1);

	emit phenotype_changed();
}

const icon *population_unit::get_small_icon() const
{
	return this->get_type()->get_phenotype_small_icon(this->get_phenotype());
}

void population_unit::set_country(const metternich::country *country)
{
	if (country == this->get_country()) {
		return;
	}

	this->country = country;

	emit country_changed();
}

const province *population_unit::get_province() const
{
	if (this->get_settlement() == nullptr) {
		return nullptr;
	}

	return this->get_settlement()->get_game_data()->get_province();
}

void population_unit::set_settlement(const site *settlement)
{
	if (settlement == this->get_settlement()) {
		return;
	}

	const province *old_province = this->get_province();

	this->settlement = settlement;

	emit settlement_changed();

	const province *province = this->get_province();

	if (old_province != province) {
		emit province_changed();
	}
}

void population_unit::set_ideology(const metternich::ideology *ideology)
{
	if (ideology == this->get_ideology()) {
		return;
	}

	if (this->get_ideology() != nullptr) {
		this->get_country()->get_game_data()->change_population_ideology_count(this->get_ideology(), -1);
	}

	this->ideology = ideology;

	if (this->get_ideology() != nullptr) {
		this->get_country()->get_game_data()->change_population_ideology_count(this->get_ideology(), 1);
	}
}

void population_unit::choose_ideology()
{
	std::vector<const metternich::ideology *> potential_ideologies;

	for (const metternich::ideology *ideology : ideology::get_all()) {
		if (ideology->get_conditions() != nullptr && !ideology->get_conditions()->check(this, read_only_context(this))) {
			continue;
		}

		const int weight = ideology->get_weight_factor()->calculate(this).to_int();

		for (int i = 0; i < weight; ++i) {
			potential_ideologies.push_back(ideology);
		}
	}

	if (!potential_ideologies.empty()) {
		this->set_ideology(vector::get_random(potential_ideologies));
	}
}

void population_unit::migrate_to(const site *settlement)
{
	qunique_ptr<population_unit> unique_ptr = this->get_country()->get_game_data()->pop_population_unit(this);
	this->get_province()->get_game_data()->remove_population_unit(this);
	this->get_settlement()->get_game_data()->remove_population_unit(this);

	assert_throw(unique_ptr != nullptr);

	const metternich::country *country = settlement->get_game_data()->get_owner();
	assert_throw(country != nullptr);

	country->get_game_data()->add_population_unit(std::move(unique_ptr));
	this->set_country(country);

	const province *province = settlement->get_game_data()->get_province();
	province->get_game_data()->add_population_unit(this);

	settlement->get_game_data()->add_population_unit(this);
	this->set_settlement(settlement);
}

}
