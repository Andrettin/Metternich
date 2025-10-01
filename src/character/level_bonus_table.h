#pragma once

#include "database/data_type.h"
#include "database/data_entry.h"

namespace metternich {

class level_bonus_table final : public data_entry, public data_type<level_bonus_table>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "level_bonus_table";
	static constexpr const char property_class_identifier[] = "metternich::level_bonus_table*";
	static constexpr const char database_folder[] = "level_bonus_tables";

	explicit level_bonus_table(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void check() const override;

	const std::map<int, int> &get_bonus_per_level() const
	{
		return this->bonus_per_level;
	}

	int get_bonus_per_level(const int level) const
	{
		const auto find_iterator = this->bonus_per_level.find(level);
		if (find_iterator != this->bonus_per_level.end()) {
			return find_iterator->second;
		}

		return 0;
	}

signals:
	void changed();

private:
	std::map<int, int> bonus_per_level;
};

}
