#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class country;
class culture;
class historical_military_unit_history;
class military_unit_type;
class phenotype;
class population_type;
class province;
class religion;

class historical_military_unit final : public named_data_entry, public data_type<historical_military_unit>
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_type* type MEMBER type)
	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(metternich::province* home_province MEMBER home_province)
	Q_PROPERTY(metternich::population_type* population_type MEMBER population_type)
	Q_PROPERTY(metternich::culture* culture MEMBER culture)
	Q_PROPERTY(metternich::religion* religion MEMBER religion)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype)

public:
	static constexpr const char class_identifier[] = "historical_military_unit";
	static constexpr const char property_class_identifier[] = "metternich::historical_military_unit*";
	static constexpr const char database_folder[] = "military_units";
	static constexpr bool history_enabled = true;

	explicit historical_military_unit(const std::string &identifier);
	~historical_military_unit();

	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	historical_military_unit_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const military_unit_type *get_type() const
	{
		return this->type;
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	const province *get_home_province() const
	{
		return this->home_province;
	}

	const metternich::population_type *get_population_type() const
	{
		return this->population_type;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

private:
	military_unit_type *type = nullptr;
	country *owner = nullptr;
	province *home_province = nullptr;
	metternich::population_type *population_type = nullptr;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	metternich::phenotype *phenotype = nullptr;
	qunique_ptr<historical_military_unit_history> history;
};

}
