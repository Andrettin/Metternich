#pragma once

#include "database/data_type.h"
#include "species/taxon_base.h"

namespace metternich {

class phenotype;
class taxon;
enum class geological_era;
enum class taxonomic_rank;

class species final : public taxon_base, public data_type<species>
{
	Q_OBJECT

	Q_PROPERTY(QString specific_name READ get_specific_name_qstring)
	Q_PROPERTY(metternich::geological_era era MEMBER era READ get_era)
	Q_PROPERTY(bool sapient MEMBER sapient READ is_sapient)
	Q_PROPERTY(bool asexual MEMBER asexual READ is_asexual)
	Q_PROPERTY(bool domestic MEMBER domestic READ is_domestic)

public:
	static constexpr const char class_identifier[] = "species";
	static constexpr const char property_class_identifier[] = "metternich::species*";
	static constexpr const char database_folder[] = "species";

	static std::map<const taxon *, int> get_supertaxon_counts(const std::vector<const species *> &source_species_list, const std::vector<const taxon *> &taxons);
	static std::vector<std::string> get_name_list(const std::vector<const species *> &species_list);

	explicit species(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	virtual taxonomic_rank get_rank() const override;

	const std::string &get_specific_name() const
	{
		return this->specific_name;
	}

	QString get_specific_name_qstring() const
	{
		return QString::fromStdString(this->specific_name);
	}

	Q_INVOKABLE void set_specific_name(const std::string &name)
	{
		this->specific_name = name;
	}

	std::string get_scientific_name() const;

	virtual const std::string &get_common_name() const override
	{
		return this->get_name();
	}

	geological_era get_era() const
	{
		return this->era;
	}

	world *get_homeworld() const
	{
		return this->homeworld;
	}

	bool is_sapient() const
	{
		return this->sapient;
	}

	bool is_prehistoric() const;

	bool is_asexual() const
	{
		return this->asexual;
	}

	bool is_domestic() const
	{
		return this->domestic;
	}

	const std::vector<const species *> &get_pre_evolutions() const
	{
		return this->pre_evolutions;
	}

	const std::vector<const species *> &get_evolutions() const
	{
		return this->evolutions;
	}

	const std::vector<const phenotype *> &get_phenotypes() const
	{
		return this->phenotypes;
	}

	void add_phenotype(const phenotype *phenotype)
	{
		this->phenotypes.push_back(phenotype);
	}

private:
	std::string specific_name;
	geological_era era;
	world *homeworld = nullptr;
	bool sapient = false;
	bool asexual = false;
	bool domestic = false;
	std::vector<const species *> pre_evolutions; //species from which this one can evolve
	std::vector<const species *> evolutions; //species to which this one can evolve
	std::vector<const phenotype *> phenotypes;
};

}
