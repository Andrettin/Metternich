#include "metternich.h"

#include "population/population_unit.h"

#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
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

population_unit::population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const int64_t size, const metternich::site *site)
	: type(type), culture(culture), religion(religion), phenotype(phenotype), size(size), site(site)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_size() > 0);

	if (!vector::contains(this->get_culture()->get_species(), this->get_species())) {
		throw std::runtime_error(std::format("Tried to create a population unit with a species (\"{}\") which is not allowed for its culture (\"{}\").", this->get_species()->get_identifier(), this->get_culture()->get_identifier()));
	}

	this->set_country(site->get_game_data()->get_owner());
	assert_throw(this->get_country() != nullptr);

	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

void population_unit::do_promotion()
{
	const factor<population_unit> *promotion_chance = defines::get()->get_promotion_chance();
	const decimillesimal_int promotion_chance_result = promotion_chance->calculate(this);

	decimillesimal_int promotion_rate = defines::get()->get_base_monthly_promotion_rate() * game::get()->get_current_months_per_turn();
	promotion_rate *= promotion_chance_result;

	if (promotion_rate == 0) {
		return;
	}

	const population_type *best_promotion_type = nullptr;
	decimillesimal_int best_promotion_factor_result;

	for (const auto &[promotion_type, promotion_factor] : this->get_type()->get_promotion_factors()) {
		if (!this->get_site()->get_game_data()->can_have_population_type(promotion_type)) {
			continue;
		}

		const decimillesimal_int promotion_factor_result = promotion_factor->calculate(this);
		if (promotion_factor_result > 0 && promotion_factor_result > best_promotion_factor_result) {
			best_promotion_factor_result = promotion_factor_result;
			best_promotion_type = promotion_type;
		}
	}

	if (best_promotion_type == nullptr) {
		return;
	}

	const int64_t promoted_size = (this->get_size() * promotion_rate / 100).to_int64();

	this->get_site()->get_game_data()->change_population(this->get_type(), this->get_culture(), this->get_religion(), this->get_phenotype(), -promoted_size);

	this->get_site()->get_game_data()->change_population(best_promotion_type, this->get_culture(), this->get_religion(), this->get_phenotype(), promoted_size);
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

	this->get_site()->get_game_data()->get_population()->change_type_size(this->get_type(), -this->get_size());

	this->type = type;

	this->get_site()->get_game_data()->get_population()->change_type_size(this->get_type(), this->get_size());

	emit type_changed();
}

void population_unit::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	assert_throw(culture != nullptr);
	assert_throw(vector::contains(culture->get_species(), this->get_species()));

	this->get_site()->get_game_data()->get_population()->change_culture_size(this->get_culture(), -this->get_size());

	this->culture = culture;

	this->get_site()->get_game_data()->get_population()->change_culture_size(this->get_culture(), this->get_size());

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

	this->get_site()->get_game_data()->get_population()->change_religion_size(this->get_religion(), -this->get_size());

	this->religion = religion;

	this->get_site()->get_game_data()->get_population()->change_religion_size(this->get_religion(), this->get_size());

	emit religion_changed();
}

void population_unit::set_phenotype(const metternich::phenotype *phenotype)
{
	if (phenotype == this->get_phenotype()) {
		return;
	}

	assert_throw(phenotype != nullptr);
	assert_throw(vector::contains(this->get_culture()->get_species(), phenotype->get_species()));

	this->get_site()->get_game_data()->get_population()->change_phenotype_size(this->get_phenotype(), -this->get_size());

	this->phenotype = phenotype;

	this->get_site()->get_game_data()->get_population()->change_phenotype_size(this->get_phenotype(), this->get_size());

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

void population_unit::set_country(const metternich::domain *domain)
{
	if (domain == this->get_country()) {
		return;
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->remove_population_unit(this);
	}

	this->domain = domain;

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

	const metternich::domain *domain = site ? site->get_game_data()->get_owner() : nullptr;
	this->set_country(domain);
}

void population_unit::set_size(const int64_t size)
{
	if (size == this->get_size()) {
		return;
	}

	assert_throw(this->get_site() != nullptr);
	this->get_site()->get_game_data()->get_population()->on_population_unit_lost(this);

	this->size = size;

	this->get_site()->get_game_data()->get_population()->on_population_unit_gained(this);

	emit size_changed();
}

bool population_unit::is_food_producer() const
{
	if (this->get_type()->get_output_commodity() != nullptr) {
		return this->get_type()->get_output_commodity()->is_food();
	}

	return false;
}

}
