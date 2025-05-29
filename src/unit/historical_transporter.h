#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("species/phenotype.h")
Q_MOC_INCLUDE("unit/transporter_type.h")

namespace metternich {

class country;
class historical_transporter_history;
class phenotype;
class transporter_type;

class historical_transporter final : public named_data_entry, public data_type<historical_transporter>
{
	Q_OBJECT

	Q_PROPERTY(metternich::transporter_type* type MEMBER type)
	Q_PROPERTY(metternich::country* country MEMBER country)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype)
	Q_PROPERTY(int quantity MEMBER quantity READ get_quantity)

public:
	static constexpr const char class_identifier[] = "historical_transporter";
	static constexpr const char property_class_identifier[] = "metternich::historical_transporter*";
	static constexpr const char database_folder[] = "transporters";
	static constexpr bool history_enabled = true;

	explicit historical_transporter(const std::string &identifier);
	~historical_transporter();

	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	historical_transporter_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const transporter_type *get_type() const
	{
		return this->type;
	}

	const metternich::country *get_country() const
	{
		return this->country;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	int get_quantity() const
	{
		return this->quantity;
	}

private:
	transporter_type *type = nullptr;
	metternich::country *country = nullptr;
	metternich::phenotype *phenotype = nullptr;
	int quantity = 1;
	qunique_ptr<historical_transporter_history> history;
};

}
