#pragma once

#include "domain/culture_container.h"
#include "population/population_type_container.h"
#include "religion/religion_container.h"
#include "species/phenotype_container.h"
#include "util/centesimal_int.h"

namespace metternich {

class population_unit;

//a class which keeps track of population counts
class population final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_unit_count_changed)
	Q_PROPERTY(qint64 size READ get_size NOTIFY size_changed)
	Q_PROPERTY(QVariantList type_sizes READ get_type_sizes_qvariant_list NOTIFY type_sizes_changed)
	Q_PROPERTY(QVariantList culture_sizes READ get_culture_sizes_qvariant_list NOTIFY culture_sizes_changed)
	Q_PROPERTY(QVariantList religion_sizes READ get_religion_sizes_qvariant_list NOTIFY religion_sizes_changed)
	Q_PROPERTY(QVariantList phenotype_sizes READ get_phenotype_sizes_qvariant_list NOTIFY phenotype_sizes_changed)
	Q_PROPERTY(int literacy_rate READ get_literacy_rate NOTIFY type_sizes_changed)
	Q_PROPERTY(int literate_size READ get_literate_size NOTIFY type_sizes_changed)

public:
	int get_population_unit_count() const
	{
		return this->population_unit_count;
	}

	void change_population_unit_count(const int change);

	int64_t get_size() const
	{
		return this->size;
	}

	void change_size(const int64_t change);

	const population_type_map<int64_t> &get_type_sizes() const
	{
		return this->type_sizes;
	}

	QVariantList get_type_sizes_qvariant_list() const;

	Q_INVOKABLE qint64 get_type_size(const metternich::population_type *type) const
	{
		const auto find_iterator = this->get_type_sizes().find(type);
		if (find_iterator != this->get_type_sizes().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_type_size(const population_type *type, const int64_t change);

	const culture_map<int64_t> &get_culture_sizes() const
	{
		return this->culture_sizes;
	}

	QVariantList get_culture_sizes_qvariant_list() const;
	void change_culture_size(const culture *culture, const int64_t change);

	const culture *get_main_culture() const
	{
		return this->main_culture;
	}

	void set_main_culture(const culture *culture)
	{
		if (culture == this->main_culture) {
			return;
		}

		this->main_culture = culture;
		emit main_culture_changed(culture);
	}

	void calculate_main_culture()
	{
		const culture *main_culture = nullptr;
		int64_t best_size = 0;

		for (const auto &[culture, size] : this->get_culture_sizes()) {
			if (size > best_size) {
				main_culture = culture;
				best_size = size;
			}
		}

		this->set_main_culture(main_culture);
	}

	const religion_map<int64_t> &get_religion_sizes() const
	{
		return this->religion_sizes;
	}

	QVariantList get_religion_sizes_qvariant_list() const;
	void change_religion_size(const religion *religion, const int64_t change);

	const religion *get_main_religion() const
	{
		return this->main_religion;
	}

	void set_main_religion(const religion *religion)
	{
		if (religion == this->main_religion) {
			return;
		}

		this->main_religion = religion;
		emit main_religion_changed(religion);
	}

	void calculate_main_religion()
	{
		const religion *main_religion = nullptr;
		int64_t best_size = 0;

		for (const auto &[religion, size] : this->get_religion_sizes()) {
			if (size > best_size) {
				main_religion = religion;
				best_size = size;
			}
		}

		this->set_main_religion(main_religion);
	}

	const phenotype_map<int64_t> &get_phenotype_sizes() const
	{
		return this->phenotype_sizes;
	}

	QVariantList get_phenotype_sizes_qvariant_list() const;
	void change_phenotype_size(const phenotype *phenotype, const int64_t change);

	std::vector<const phenotype *> get_weighted_phenotypes_for_culture(const culture *culture) const;

	int64_t get_literate_size() const
	{
		return this->literate_size;
	}

	int get_literacy_rate() const
	{
		if (this->get_population_unit_count() == 0) {
			return 0;
		}

		return static_cast<int>(this->get_literate_size() * 100 / this->get_size());
	}

	void add_upper_population(population *upper_population)
	{
		if (upper_population != nullptr) {
			upper_population->change_from(this, 1);
		}

		this->upper_populations.push_back(upper_population);
	}

	void remove_upper_population(population *upper_population)
	{
		if (upper_population != nullptr) {
			upper_population->change_from(this, -1);
		}

		std::erase(this->upper_populations, upper_population);
	}

	void change_from(population *other_population, const int change)
	{
		this->change_population_unit_count(other_population->get_population_unit_count() * change);
		this->change_size(other_population->get_size() * change);

		for (const auto &[type, size] : other_population->get_type_sizes()) {
			this->change_type_size(type, size * change);
		}

		for (const auto &[culture, size] : other_population->get_culture_sizes()) {
			this->change_culture_size(culture, size * change);
		}

		for (const auto &[religion, size] : other_population->get_religion_sizes()) {
			this->change_religion_size(religion, size * change);
		}

		for (const auto &[phenotype, size] : other_population->get_phenotype_sizes()) {
			this->change_phenotype_size(phenotype, size * change);
		}
	}

	void on_population_unit_gained(const population_unit *population_unit, const int multiplier = 1);

	void on_population_unit_lost(const population_unit *population_unit)
	{
		this->on_population_unit_gained(population_unit, -1);
	}

signals:
	void population_unit_count_changed();
	void size_changed();
	void type_sizes_changed();
	void culture_sizes_changed();
	void main_culture_changed(const culture *culture);
	void religion_sizes_changed();
	void main_religion_changed(const religion *religion);
	void phenotype_sizes_changed();
	void population_unit_gained(const population_unit *population_unit, const int multiplier);

private:
	int population_unit_count = 0;
	int64_t size = 0;
	population_type_map<int64_t> type_sizes;
	culture_map<int64_t> culture_sizes;
	const culture *main_culture = nullptr;
	religion_map<int64_t> religion_sizes;
	const religion *main_religion = nullptr;
	phenotype_map<int64_t> phenotype_sizes;
	int64_t literate_size = 0;
	std::vector<population *> upper_populations;
};

}
