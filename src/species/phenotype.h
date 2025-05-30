#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class species;

class phenotype final : public named_data_entry, public data_type<phenotype>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(metternich::species* species MEMBER species NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "phenotype";
	static constexpr const char property_class_identifier[] = "metternich::phenotype*";
	static constexpr const char database_folder[] = "phenotypes";

	explicit phenotype(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		this->color = color;
	}

	metternich::species *get_species() const
	{
		return this->species;
	}

	void set_species(metternich::species *species)
	{
		this->species = species;
	}

signals:
	void changed();

private:
	QColor color;
	metternich::species *species = nullptr;
};

}
