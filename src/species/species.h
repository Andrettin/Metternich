#pragma once

#include "database/data_type.h"
#include "species/taxon_base.h"

namespace metternich {

class character;
class culture;
class phenotype;
class taxon;
enum class geological_era;
enum class taxonomic_rank;

template <typename scope_type>
class modifier;

class species final : public taxon_base, public data_type<species>
{
	Q_OBJECT

	Q_PROPERTY(QString specific_name READ get_specific_name_qstring)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(metternich::geological_era era MEMBER era READ get_era)
	Q_PROPERTY(bool sapient MEMBER sapient READ is_sapient)
	Q_PROPERTY(bool asexual MEMBER asexual READ is_asexual)
	Q_PROPERTY(bool domestic MEMBER domestic READ is_domestic)

public:
	static constexpr const char class_identifier[] = "species";
	static constexpr const char property_class_identifier[] = "metternich::species*";
	static constexpr const char database_folder[] = "species";

	static const std::set<std::string> database_dependencies;

	static void process_database(const bool definition, const data_module_map<std::vector<gsml_data>> &gsml_data_to_process);

	static std::map<const taxon *, int> get_supertaxon_counts(const std::vector<const species *> &source_species_list, const std::vector<const taxon *> &taxons);
	static std::vector<std::string> get_name_list(const std::vector<const species *> &species_list);

	explicit species(const std::string &identifier);
	~species();

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

	const QColor &get_color() const
	{
		return this->color;
	}

	geological_era get_era() const
	{
		return this->era;
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

	const std::vector<const culture *> &get_cultures() const
	{
		return this->cultures;
	}

	void add_culture(const culture *culture)
	{
		this->cultures.push_back(culture);
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

private:
	std::string specific_name;
	QColor color;
	geological_era era;
	bool sapient = false;
	bool asexual = false;
	bool domestic = false;
	std::vector<const species *> pre_evolutions; //species from which this one can evolve
	std::vector<const species *> evolutions; //species to which this one can evolve
	std::vector<const phenotype *> phenotypes;
	std::vector<const culture *> cultures;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
};

}
