#include "metternich.h"

#include "population/population.h"

#include "culture/culture.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "game/game.h"
#include "population/population_strata.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/religion.h"
#include "species/phenotype.h"
#include "util/assert_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"

namespace metternich {

void population::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "population_unit_count") {
		this->population_unit_count = std::stoi(value);
	} else if (key == "size") {
		this->size = std::stoll(value);
	} else if (key == "main_culture") {
		this->main_culture = culture::get(value);
	} else if (key == "main_religion") {
		this->main_religion = religion::get(value);
	} else if (key == "literate_size") {
		this->literate_size = std::stoll(value);
	} else {
		throw std::runtime_error(std::format("Invalid population property: \"{}\".", key));
	}
}

void population::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "type_sizes") {
		scope.for_each_property([this](const gsml_property &property) {
			this->type_sizes[population_type::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else if (tag == "culture_sizes") {
		scope.for_each_property([this](const gsml_property &property) {
			this->culture_sizes[culture::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else if (tag == "religion_sizes") {
		scope.for_each_property([this](const gsml_property &property) {
			this->religion_sizes[religion::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else if (tag == "phenotype_sizes") {
		scope.for_each_property([this](const gsml_property &property) {
			this->phenotype_sizes[phenotype::get(property.get_key())] = std::stoll(property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid population scope: \"{}\".", tag));
	}
}

gsml_data population::to_gsml_data(const std::string_view tag) const
{
	gsml_data data(!tag.empty() ? std::string(tag) : "population");

	data.add_property("population_unit_count", std::to_string(this->population_unit_count));
	data.add_property("size", std::to_string(this->size));
	data.add_property("literate_size", std::to_string(this->literate_size));

	if (this->main_culture != nullptr) {
		data.add_property("main_culture", this->main_culture->get_identifier());
	}

	if (this->main_religion != nullptr) {
		data.add_property("main_religion", this->main_religion->get_identifier());
	}

	if (!this->type_sizes.empty()) {
		gsml_data type_sizes_data("type_sizes");
		for (const auto &[population_type, type_size] : this->type_sizes) {
			type_sizes_data.add_property(population_type->get_identifier(), std::to_string(type_size));
		}
		data.add_child(std::move(type_sizes_data));
	}

	if (!this->culture_sizes.empty()) {
		gsml_data culture_sizes_data("culture_sizes");
		for (const auto &[culture, culture_size] : this->culture_sizes) {
			culture_sizes_data.add_property(culture->get_identifier(), std::to_string(culture_size));
		}
		data.add_child(std::move(culture_sizes_data));
	}

	if (!this->religion_sizes.empty()) {
		gsml_data religion_sizes_data("religion_sizes");
		for (const auto &[religion, religion_size] : this->religion_sizes) {
			religion_sizes_data.add_property(religion->get_identifier(), std::to_string(religion_size));
		}
		data.add_child(std::move(religion_sizes_data));
	}

	if (!this->phenotype_sizes.empty()) {
		gsml_data phenotype_sizes_data("phenotype_sizes");
		for (const auto &[phenotype, phenotype_size] : this->phenotype_sizes) {
			phenotype_sizes_data.add_property(phenotype->get_identifier(), std::to_string(phenotype_size));
		}
		data.add_child(std::move(phenotype_sizes_data));
	}

	return data;
}

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

	assert_throw(this->size >= 0);

	for (population *upper_population : this->upper_populations) {
		upper_population->change_size(change);
	}

	if (game::get()->is_running()) {
		emit size_changed();
	}
}

int64_t population::get_weighted_size() const
{
	int64_t weighted_population_size = 0;

	for (const auto &[population_type, population_type_size] : this->get_type_sizes()) {
		int64_t weighted_population_type_size = population_type_size;
		weighted_population_type_size *= get_population_strata_income_weight(population_type->get_strata());
		weighted_population_size += weighted_population_type_size;
	}

	return weighted_population_size;
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

	for (population *upper_population : this->upper_populations) {
		upper_population->change_type_size(type, change);
	}

	if (game::get()->is_running()) {
		emit type_sizes_changed();
	}

	emit type_size_changed(type, change);
}

QVariantList population::get_culture_sizes_qvariant_list() const
{
	QVariantList sizes = archimedes::map::to_qvariant_list(this->get_culture_sizes());
	std::sort(sizes.begin(), sizes.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toLongLong() > rhs.toMap().value("value").toLongLong();
	});
	return sizes;
}

int64_t population::get_culture_size(const culture *culture) const
{
	const auto find_iterator = this->get_culture_sizes().find(culture);
	if (find_iterator != this->get_culture_sizes().end()) {
		return find_iterator->second;
	}

	return 0;
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

phenotype_map<int64_t> population::get_phenotype_sizes_for_culture(const culture *culture) const
{
	assert_throw(culture != nullptr);

	phenotype_map<int64_t> phenotype_sizes = this->get_phenotype_sizes();

	std::erase_if(phenotype_sizes, [culture](const auto &element) {
		const auto &[key, value] = element;
		return !vector::contains(culture->get_species(), key->get_species());
	});

	return phenotype_sizes;
}

void population::change_literate_size(const int64_t change)
{
	if (change == 0) {
		return;
	}

	this->literate_size += change;

	assert_throw(this->literate_size >= 0);

	for (population *upper_population : this->upper_populations) {
		upper_population->change_literate_size(change);
	}

	if (game::get()->is_running()) {
		emit literate_size_changed();
	}
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
	this->change_literate_size(population_unit->get_literate_size() * multiplier);

	emit population_unit_gained(population_unit, multiplier);
}

}

#include "moc_population.cpp"
