#pragma once

#include "database/data_type.h"
#include "domain/culture_base.h"

namespace metternich {

class character;
class cultural_group;
class population_unit;
class species;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class culture final : public culture_base, public data_type<culture>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(bool surname_first MEMBER surname_first READ is_surname_first NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "culture";
	static constexpr const char property_class_identifier[] = "metternich::culture*";
	static constexpr const char database_folder[] = "cultures";
	static constexpr bool history_enabled = true;

	explicit culture(const std::string &identifier);
	~culture();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	using culture_base::get_group;

	const QColor &get_color() const
	{
		return this->color;
	}

	bool is_surname_first() const
	{
		return this->surname_first;
	}

	const std::vector<const metternich::species *> &get_species() const
	{
		return this->species;
	}

	std::vector<const phenotype *> get_weighted_phenotypes() const;

	const std::vector<const culture *> &get_derived_cultures() const
	{
		return this->derived_cultures;
	}

	const and_condition<population_unit> *get_derivation_conditions() const
	{
		return this->derivation_conditions.get();
	}

	const metternich::modifier<const character> *get_character_modifier() const
	{
		return this->character_modifier.get();
	}

signals:
	void changed();

private:
	QColor color;
	bool surname_first = false;
	std::vector<const metternich::species *> species; //species which can have this culture
	std::vector<const culture *> derived_cultures;
	std::unique_ptr<const and_condition<population_unit>> derivation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> character_modifier;
};

}
