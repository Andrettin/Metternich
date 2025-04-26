#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("population/phenotype.h")
Q_MOC_INCLUDE("unit/civilian_unit_type.h")

namespace metternich {

class civilian_unit_type;
class country;
class historical_civilian_unit_history;
class phenotype;

class historical_civilian_unit final : public named_data_entry, public data_type<historical_civilian_unit>
{
	Q_OBJECT

	Q_PROPERTY(metternich::civilian_unit_type* type MEMBER type)
	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype)

public:
	static constexpr const char class_identifier[] = "historical_civilian_unit";
	static constexpr const char property_class_identifier[] = "metternich::historical_civilian_unit*";
	static constexpr const char database_folder[] = "civilian_units";
	static constexpr bool history_enabled = true;

	explicit historical_civilian_unit(const std::string &identifier);
	~historical_civilian_unit();

	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	historical_civilian_unit_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const civilian_unit_type *get_type() const
	{
		return this->type;
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

private:
	civilian_unit_type *type = nullptr;
	country *owner = nullptr;
	metternich::phenotype *phenotype = nullptr;
	qunique_ptr<historical_civilian_unit_history> history;
};

}
