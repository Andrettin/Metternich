#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;
class holding_type;
class wonder;

class building_slot_type final : public named_data_entry, public data_type<building_slot_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "building_slot_type";
	static constexpr const char property_class_identifier[] = "metternich::building_slot_type*";
	static constexpr const char database_folder[] = "building_slot_types";

public:
	explicit building_slot_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const std::vector<const building_type *> &get_building_types_for_holding_type(const holding_type *holding_type) const
	{
		const auto find_iterator = this->building_types_by_holding_type.find(holding_type);

		if (find_iterator != this->building_types_by_holding_type.end()) {
			return find_iterator->second;
		}

		static const std::vector<const building_type *> empty_vector;
		return empty_vector;
	}

	void add_building_type(const building_type *building_type);

	const std::vector<const wonder *> &get_wonders() const
	{
		return this->wonders;
	}

	void add_wonder(const wonder *wonder)
	{
		this->wonders.push_back(wonder);
	}

signals:
	void changed();

private:
	data_entry_map<holding_type, std::vector<const building_type *>> building_types_by_holding_type;
	std::vector<const wonder *> wonders;
};

}
