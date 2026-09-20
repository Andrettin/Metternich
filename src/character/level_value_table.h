#pragma once

#include "database/data_type.h"
#include "database/data_entry.h"
#include "util/dice.h"

namespace metternich {

class level_value_table final : public data_entry, public data_type<level_value_table>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "level_value_table";
	static constexpr const char property_class_identifier[] = "metternich::level_value_table*";
	static constexpr const char database_folder[] = "level_value_tables";

	explicit level_value_table(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void check() const override;

	const std::variant<int, dice> &get_value_variant_for_level(const int level) const
	{
		const auto find_iterator = this->value_per_level.find(level);
		if (find_iterator != this->value_per_level.end()) {
			return find_iterator->second;
		}

		static const std::variant<int, dice> zero = 0;
		return zero;
	}

	int get_value_for_level(const int level) const
	{
		return std::get<int>(this->get_value_variant_for_level(level));
	}

signals:
	void changed();

private:
	std::map<int, std::variant<int, dice>> value_per_level;
};

}
