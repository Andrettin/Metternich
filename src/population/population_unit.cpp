#include "metternich.h"

#include "population/population_unit.h"

#include "domain/country.h"
#include "domain/country_game_data.h"
#include "domain/culture.h"
#include "domain/ideology.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "population/population.h"
#include "population/population_type.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

population_unit::population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *site)
	: type(type), culture(culture), religion(religion), phenotype(phenotype), site(site)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	if (!vector::contains(this->get_culture()->get_species(), this->get_species())) {
		throw std::runtime_error(std::format("Tried to create a population unit with a species (\"{}\") which is not allowed for its culture (\"{}\").", this->get_species()->get_identifier(), this->get_culture()->get_identifier()));
	}

	this->set_country(site->get_game_data()->get_owner());
	assert_throw(this->get_country() != nullptr);

	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

std::string population_unit::get_scope_name() const
{
	return std::format("{} {}", this->get_culture()->get_name(), this->get_type()->get_name());
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

	assert_throw(type != nullptr);

	this->get_site()->get_game_data()->get_population()->change_type_count(this->get_type(), -1);

	this->type = type;

	this->get_site()->get_game_data()->get_population()->change_type_count(this->get_type(), 1);

	emit type_changed();
}

void population_unit::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	assert_throw(culture != nullptr);
	assert_throw(vector::contains(culture->get_species(), this->get_species()));

	this->get_site()->get_game_data()->get_population()->change_culture_count(this->get_culture(), -1);

	this->culture = culture;

	this->get_site()->get_game_data()->get_population()->change_culture_count(this->get_culture(), 1);

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

	this->get_site()->get_game_data()->get_population()->change_religion_count(this->get_religion(), -1);

	this->religion = religion;

	this->get_site()->get_game_data()->get_population()->change_religion_count(this->get_religion(), 1);

	emit religion_changed();
}

void population_unit::set_phenotype(const metternich::phenotype *phenotype)
{
	if (phenotype == this->get_phenotype()) {
		return;
	}

	assert_throw(phenotype != nullptr);
	assert_throw(vector::contains(this->get_culture()->get_species(), phenotype->get_species()));

	this->get_site()->get_game_data()->get_population()->change_phenotype_count(this->get_phenotype(), -1);

	this->phenotype = phenotype;

	this->get_site()->get_game_data()->get_population()->change_phenotype_count(this->get_phenotype(), 1);

	emit phenotype_changed();
}

const species *population_unit::get_species() const
{
	return this->get_phenotype()->get_species();
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

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->remove_population_unit(this);
	}

	this->country = country;

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->add_population_unit(this);
	}

	emit country_changed();
}

const province *population_unit::get_province() const
{
	if (this->get_site() == nullptr) {
		return nullptr;
	}

	return this->get_site()->get_game_data()->get_province();
}

void population_unit::set_site(const metternich::site *site)
{
	if (site == this->get_site()) {
		return;
	}

	const province *old_province = this->get_province();

	this->site = site;

	emit site_changed();

	const province *province = this->get_province();

	if (old_province != province) {
		emit province_changed();
	}

	const metternich::country *country = site ? site->get_game_data()->get_owner() : nullptr;
	this->set_country(country);
}

void population_unit::set_ideology(const metternich::ideology *ideology)
{
	if (ideology == this->get_ideology()) {
		return;
	}

	if (this->get_ideology() != nullptr) {
		this->get_site()->get_game_data()->get_population()->change_ideology_count(this->get_ideology(), -1);
	}

	this->ideology = ideology;

	if (this->get_ideology() != nullptr) {
		this->get_site()->get_game_data()->get_population()->change_ideology_count(this->get_ideology(), 1);
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

bool population_unit::is_food_producer() const
{
	if (this->get_type()->get_output_commodity() != nullptr) {
		return this->get_type()->get_output_commodity()->is_food();
	}

	return false;
}

}
