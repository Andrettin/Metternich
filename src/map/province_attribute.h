#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class province;

template <typename scope_type>
class modifier;

class province_attribute final : public named_data_entry, public data_type<province_attribute>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province_attribute";
	static constexpr const char property_class_identifier[] = "metternich::province_attribute*";
	static constexpr const char database_folder[] = "province_attributes";

	explicit province_attribute(const std::string &identifier);
	~province_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const modifier<const province> *get_value_modifier(const int value) const
	{
		const auto find_iterator = this->value_modifiers.find(value);
		if (find_iterator != this->value_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

signals:
	void changed();

private:
	std::map<int, std::unique_ptr<modifier<const province>>> value_modifiers; //the province modifiers applied for each value; these are cumulative
};

}
