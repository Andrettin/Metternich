#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class country;
class province;
enum class office_type;

template <typename scope_type>
class condition;

class office final : public named_data_entry, public data_type<office>
{
	Q_OBJECT

	Q_PROPERTY(metternich::office_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(int limit MEMBER limit READ get_limit NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "office";
	static constexpr const char property_class_identifier[] = "metternich::office*";
	static constexpr const char database_folder[] = "offices";

	explicit office(const std::string &identifier);
	~office();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	office_type get_type() const
	{
		return this->type;
	}

	int get_limit() const
	{
		return this->limit;
	}

	const condition<country> *get_country_conditions() const
	{
		return this->country_conditions.get();
	}

	const condition<province> *get_province_conditions() const
	{
		return this->province_conditions.get();
	}

	const condition<character> *get_character_conditions() const
	{
		return this->character_conditions.get();
	}

signals:
	void changed();

private:
	office_type type;
	int limit = 1;
	std::unique_ptr<const condition<country>> country_conditions;
	std::unique_ptr<const condition<province>> province_conditions;
	std::unique_ptr<const condition<character>> character_conditions;
};

}
