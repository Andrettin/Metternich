#pragma once

#include "country/culture_container.h"
#include "country/ideology_container.h"
#include "country/religion_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"
#include "util/fractional_int.h"

namespace metternich {

class population_unit;

//a class which keeps track of population counts
class population final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int population_unit_count READ get_population_unit_count NOTIFY population_unit_count_changed)
	Q_PROPERTY(qint64 size READ get_size NOTIFY size_changed)
	Q_PROPERTY(QVariantList type_counts READ get_type_counts_qvariant_list NOTIFY type_counts_changed)
	Q_PROPERTY(QVariantList culture_counts READ get_culture_counts_qvariant_list NOTIFY culture_counts_changed)
	Q_PROPERTY(QVariantList religion_counts READ get_religion_counts_qvariant_list NOTIFY religion_counts_changed)
	Q_PROPERTY(QVariantList phenotype_counts READ get_phenotype_counts_qvariant_list NOTIFY phenotype_counts_changed)
	Q_PROPERTY(QVariantList ideology_counts READ get_ideology_counts_qvariant_list NOTIFY ideology_counts_changed)
	Q_PROPERTY(int average_consciousness READ get_average_consciousness_int NOTIFY consciousness_changed)
	Q_PROPERTY(int average_militancy READ get_average_militancy_int NOTIFY militancy_changed)
	Q_PROPERTY(int literacy_rate READ get_literacy_rate NOTIFY type_counts_changed)
	Q_PROPERTY(int literate_count READ get_literate_count NOTIFY type_counts_changed)

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

	const population_type_map<int> &get_type_counts() const
	{
		return this->type_counts;
	}

	QVariantList get_type_counts_qvariant_list() const;

	int get_type_count(const population_type *type) const
	{
		const auto find_iterator = this->get_type_counts().find(type);
		if (find_iterator != this->get_type_counts().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_type_count(const population_type *type, const int change);

	const culture_map<int> &get_culture_counts() const
	{
		return this->culture_counts;
	}

	QVariantList get_culture_counts_qvariant_list() const;
	void change_culture_count(const culture *culture, const int change);

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
		int best_count = 0;

		for (const auto &[culture, count] : this->get_culture_counts()) {
			if (count > best_count) {
				main_culture = culture;
				best_count = count;
			}
		}

		this->set_main_culture(main_culture);
	}

	const religion_map<int> &get_religion_counts() const
	{
		return this->religion_counts;
	}

	QVariantList get_religion_counts_qvariant_list() const;
	void change_religion_count(const religion *religion, const int change);

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
		int best_count = 0;

		for (const auto &[religion, count] : this->get_religion_counts()) {
			if (count > best_count) {
				main_religion = religion;
				best_count = count;
			}
		}

		this->set_main_religion(main_religion);
	}

	const phenotype_map<int> &get_phenotype_counts() const
	{
		return this->phenotype_counts;
	}

	QVariantList get_phenotype_counts_qvariant_list() const;
	void change_phenotype_count(const phenotype *phenotype, const int change);

	const ideology_map<int> &get_ideology_counts() const
	{
		return this->ideology_counts;
	}

	QVariantList get_ideology_counts_qvariant_list() const;
	void change_ideology_count(const ideology *ideology, const int change);

	centesimal_int get_average_consciousness() const;

	int get_average_consciousness_int() const
	{
		return this->get_average_consciousness().to_int();
	}

	const centesimal_int &get_total_consciousness() const
	{
		return this->total_consciousness;
	}

	void change_total_consciousness(const centesimal_int &change);

	centesimal_int get_average_militancy() const;

	int get_average_militancy_int() const
	{
		return this->get_average_militancy().to_int();
	}

	const centesimal_int &get_total_militancy() const
	{
		return this->total_militancy;
	}

	void change_total_militancy(const centesimal_int &change);

	int get_literate_count() const
	{
		return this->literate_count;
	}

	int get_literacy_rate() const
	{
		if (this->get_population_unit_count() == 0) {
			return 0;
		}

		return this->get_literate_count() * 100 / this->get_population_unit_count();
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

		for (const auto &[type, count] : other_population->get_type_counts()) {
			this->change_type_count(type, count * change);
		}

		for (const auto &[culture, count] : other_population->get_culture_counts()) {
			this->change_culture_count(culture, count * change);
		}

		for (const auto &[religion, count] : other_population->get_religion_counts()) {
			this->change_religion_count(religion, count * change);
		}

		for (const auto &[phenotype, count] : other_population->get_phenotype_counts()) {
			this->change_phenotype_count(phenotype, count * change);
		}

		for (const auto &[ideology, count] : other_population->get_ideology_counts()) {
			this->change_ideology_count(ideology, count * change);
		}

		this->change_total_consciousness(other_population->get_total_consciousness() * change);
		this->change_total_militancy(other_population->get_total_militancy() * change);
	}

	void on_population_unit_gained(const population_unit *population_unit, const int multiplier = 1);

	void on_population_unit_lost(const population_unit *population_unit)
	{
		this->on_population_unit_gained(population_unit, -1);
	}

signals:
	void population_unit_count_changed();
	void size_changed();
	void type_counts_changed();
	void type_count_changed(const population_type *type, const int change);
	void culture_counts_changed();
	void main_culture_changed(const culture *culture);
	void religion_counts_changed();
	void main_religion_changed(const religion *religion);
	void phenotype_counts_changed();
	void ideology_counts_changed();
	void consciousness_changed();
	void militancy_changed();

private:
	int population_unit_count = 0;
	int64_t size = 0;
	population_type_map<int> type_counts;
	culture_map<int> culture_counts;
	const culture *main_culture = nullptr;
	religion_map<int> religion_counts;
	const religion *main_religion = nullptr;
	phenotype_map<int> phenotype_counts;
	ideology_map<int> ideology_counts;
	centesimal_int total_consciousness;
	centesimal_int total_militancy;
	int literate_count = 0;
	std::vector<population *> upper_populations;
};

}
