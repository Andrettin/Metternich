#include "metternich.h"

#include "population/population.h"

#include "country/culture.h"
#include "database/defines.h"
#include "game/game.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void population::change_population_unit_count(const int change)
{
	if (change == 0) {
		return;
	}

	this->population_unit_count += change;

	for (population *upper_population : this->upper_populations) {
		upper_population->change_population_unit_count(change);
	}

	if (game::get()->is_running()) {
		emit population_unit_count_changed();
	}
}

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

	if (type->is_literate()) {
		this->literate_count += change;
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
	QVariantList counts = archimedes::map::to_qvariant_list(this->get_culture_counts());
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});
	return counts;
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

	this->calculate_main_culture();

	if (game::get()->is_running()) {
		emit culture_counts_changed();
	}
}

QVariantList population::get_religion_counts_qvariant_list() const
{
	QVariantList counts = archimedes::map::to_qvariant_list(this->get_religion_counts());
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});
	return counts;
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

	this->calculate_main_religion();

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

centesimal_int population::get_average_consciousness() const
{
	if (this->get_population_unit_count() == 0) {
		return centesimal_int(0);
	}

	return this->get_total_consciousness() / this->get_population_unit_count();
}

void population::change_total_consciousness(const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	this->total_consciousness += change;

	for (population *upper_population : this->upper_populations) {
		upper_population->change_total_consciousness(change);
	}

	if (game::get()->is_running()) {
		emit consciousness_changed();
	}
}

centesimal_int population::get_average_militancy() const
{
	if (this->get_population_unit_count() == 0) {
		return centesimal_int(0);
	}

	return this->get_total_militancy() / this->get_population_unit_count();
}

void population::change_total_militancy(const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	this->total_militancy += change;

	for (population *upper_population : this->upper_populations) {
		upper_population->change_total_militancy(change);
	}

	if (game::get()->is_running()) {
		emit militancy_changed();
	}
}

void population::on_population_unit_gained(const population_unit *population_unit, const int multiplier)
{
	this->change_population_unit_count(multiplier);
	this->change_size(static_cast<int64_t>(defines::get()->get_population_per_unit()) * multiplier);

	this->change_type_count(population_unit->get_type(), multiplier);
	this->change_culture_count(population_unit->get_culture(), multiplier);
	this->change_religion_count(population_unit->get_religion(), multiplier);
	this->change_phenotype_count(population_unit->get_phenotype(), multiplier);
	if (population_unit->get_ideology() != nullptr) {
		this->change_ideology_count(population_unit->get_ideology(), multiplier);
	}

	this->change_total_consciousness(population_unit->get_consciousness() * multiplier);
	this->change_total_militancy(population_unit->get_militancy() * multiplier);
}

}
