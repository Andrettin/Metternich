#pragma once

#include "country/culture_container.h"
#include "country/ideology_container.h"
#include "country/religion_container.h"
#include "population/phenotype_container.h"
#include "population/population_type_container.h"

namespace metternich {

class population_unit;

//a class which keeps track of population counts
class population final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(qint64 size READ get_size NOTIFY size_changed)
	Q_PROPERTY(QVariantList type_counts READ get_type_counts_qvariant_list NOTIFY type_counts_changed)
	Q_PROPERTY(QVariantList culture_counts READ get_culture_counts_qvariant_list NOTIFY culture_counts_changed)
	Q_PROPERTY(QVariantList religion_counts READ get_religion_counts_qvariant_list NOTIFY religion_counts_changed)
	Q_PROPERTY(QVariantList phenotype_counts READ get_phenotype_counts_qvariant_list NOTIFY phenotype_counts_changed)
	Q_PROPERTY(QVariantList ideology_counts READ get_ideology_counts_qvariant_list NOTIFY ideology_counts_changed)

public:
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
	void change_type_count(const population_type *type, const int change);

	const culture_map<int> &get_culture_counts() const
	{
		return this->culture_counts;
	}

	QVariantList get_culture_counts_qvariant_list() const;
	void change_culture_count(const culture *culture, const int change);

	const religion_map<int> &get_religion_counts() const
	{
		return this->religion_counts;
	}

	QVariantList get_religion_counts_qvariant_list() const;
	void change_religion_count(const religion *religion, const int change);

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


	void on_population_unit_gained(const population_unit *population_unit, const int multiplier = 1);

	void on_population_unit_lost(const population_unit *population_unit)
	{
		this->on_population_unit_gained(population_unit, -1);
	}

signals:
	void size_changed();
	void type_counts_changed();
	void culture_counts_changed();
	void religion_counts_changed();
	void phenotype_counts_changed();
	void ideology_counts_changed();

private:
	int64_t size = 0;
	population_type_map<int> type_counts;
	culture_map<int> culture_counts;
	religion_map<int> religion_counts;
	phenotype_map<int> phenotype_counts;
	ideology_map<int> ideology_counts;
};

}
