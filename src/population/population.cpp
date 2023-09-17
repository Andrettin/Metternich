#include "metternich.h"

#include "population/population.h"

#include "database/defines.h"
#include "game/game.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void population::change_size(const int64_t change)
{
	if (change == 0) {
		return;
	}

	this->size += change;

	for (population *upper_population : this->upper_populations) {
		upper_population->change_size(change);
	}

	if (game::get()->is_running()) {
		emit size_changed();
	}
}

QVariantList population::get_type_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_type_counts());
}

void population::change_type_count(const population_type *type, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->type_counts[type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->type_counts.erase(type);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_type_count(type, change);
	}

	emit type_count_changed(type, change);

	if (game::get()->is_running()) {
		emit type_counts_changed();
	}
}

QVariantList population::get_culture_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_culture_counts());
}

void population::change_culture_count(const culture *culture, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->culture_counts[culture] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->culture_counts.erase(culture);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_culture_count(culture, change);
	}

	if (game::get()->is_running()) {
		emit culture_counts_changed();
	}
}

QVariantList population::get_religion_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_religion_counts());
}

void population::change_religion_count(const metternich::religion *religion, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->religion_counts[religion] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->religion_counts.erase(religion);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_religion_count(religion, change);
	}

	if (game::get()->is_running()) {
		emit religion_counts_changed();
	}
}

QVariantList population::get_phenotype_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_phenotype_counts());
}

void population::change_phenotype_count(const phenotype *phenotype, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->phenotype_counts[phenotype] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->phenotype_counts.erase(phenotype);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_phenotype_count(phenotype, change);
	}

	if (game::get()->is_running()) {
		emit phenotype_counts_changed();
	}
}

QVariantList population::get_ideology_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_ideology_counts());
}

void population::change_ideology_count(const ideology *ideology, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->ideology_counts[ideology] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->ideology_counts.erase(ideology);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_ideology_count(ideology, change);
	}

	if (game::get()->is_running()) {
		emit ideology_counts_changed();
	}
}

void population::on_population_unit_gained(const population_unit *population_unit, const int multiplier)
{
	this->change_size(defines::get()->get_population_per_unit() * multiplier);

	this->change_type_count(population_unit->get_type(), multiplier);
	this->change_culture_count(population_unit->get_culture(), multiplier);
	this->change_religion_count(population_unit->get_religion(), multiplier);
	this->change_phenotype_count(population_unit->get_phenotype(), multiplier);
	if (population_unit->get_ideology() != nullptr) {
		this->change_ideology_count(population_unit->get_ideology(), multiplier);
	}
}

}
