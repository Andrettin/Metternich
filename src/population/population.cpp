#include "metternich.h"

#include "population/population.h"

#include "database/defines.h"
#include "domain/culture.h"
#include "game/game.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"

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

QVariantList population::get_type_sizes_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_type_sizes());
}

void population::change_type_size(const population_type *type, const int64_t change)
{
	if (change == 0) {
		return;
	}

	const int64_t size = (this->type_sizes[type] += change);

	assert_throw(size >= 0);

	if (size == 0) {
		this->type_sizes.erase(type);
	}

	if (type->is_literate()) {
		this->literate_size += change;
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_type_size(type, change);
	}

	if (game::get()->is_running()) {
		emit type_sizes_changed();
	}
}

QVariantList population::get_culture_sizes_qvariant_list() const
{
	QVariantList sizes = archimedes::map::to_qvariant_list(this->get_culture_sizes());
	std::sort(sizes.begin(), sizes.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toLongLong() > rhs.toMap().value("value").toLongLong();
	});
	return sizes;
}

void population::change_culture_size(const culture *culture, const int64_t change)
{
	if (change == 0) {
		return;
	}

	const int64_t size = (this->culture_sizes[culture] += change);

	assert_throw(size >= 0);

	if (size == 0) {
		this->culture_sizes.erase(culture);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_culture_size(culture, change);
	}

	this->calculate_main_culture();

	if (game::get()->is_running()) {
		emit culture_sizes_changed();
	}
}

QVariantList population::get_religion_sizes_qvariant_list() const
{
	QVariantList sizes = archimedes::map::to_qvariant_list(this->get_religion_sizes());
	std::sort(sizes.begin(), sizes.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toLongLong() > rhs.toMap().value("value").toLongLong();
	});
	return sizes;
}

void population::change_religion_size(const metternich::religion *religion, const int64_t change)
{
	if (change == 0) {
		return;
	}

	const int64_t size = (this->religion_sizes[religion] += change);

	assert_throw(size >= 0);

	if (size == 0) {
		this->religion_sizes.erase(religion);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_religion_size(religion, change);
	}

	this->calculate_main_religion();

	if (game::get()->is_running()) {
		emit religion_sizes_changed();
	}
}

QVariantList population::get_phenotype_sizes_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_phenotype_sizes());
}

void population::change_phenotype_size(const phenotype *phenotype, const int64_t change)
{
	if (change == 0) {
		return;
	}

	const int64_t size = (this->phenotype_sizes[phenotype] += change);

	assert_throw(size >= 0);

	if (size == 0) {
		this->phenotype_sizes.erase(phenotype);
	}

	for (population *upper_population : this->upper_populations) {
		upper_population->change_phenotype_size(phenotype, change);
	}

	if (game::get()->is_running()) {
		emit phenotype_sizes_changed();
	}
}

std::vector<const phenotype *> population::get_weighted_phenotypes_for_culture(const culture *culture) const
{
	assert_throw(culture != nullptr);

	phenotype_map<int64_t> phenotype_sizes = this->get_phenotype_sizes();

	std::erase_if(phenotype_sizes, [culture](const auto &element) {
		const auto &[key, value] = element;
		return !vector::contains(culture->get_species(), key->get_species());
	});

	return archimedes::map::to_weighted_vector(phenotype_sizes);
}

void population::on_population_unit_gained(const population_unit *population_unit, const int multiplier)
{
	this->change_population_unit_count(multiplier);

	const int64_t size = population_unit->get_size();
	this->change_size(size * multiplier);

	this->change_type_size(population_unit->get_type(), size * multiplier);
	this->change_culture_size(population_unit->get_culture(), size * multiplier);
	this->change_religion_size(population_unit->get_religion(), size * multiplier);
	this->change_phenotype_size(population_unit->get_phenotype(), size * multiplier);

	emit population_unit_gained(population_unit, multiplier);
}

}
