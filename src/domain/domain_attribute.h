#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class domain;

template <typename scope_type>
class modifier;

class domain_attribute final : public named_data_entry, public data_type<domain_attribute>
{
	Q_OBJECT

	Q_PROPERTY(bool taxable MEMBER taxable READ is_taxable NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "domain_attribute";
	static constexpr const char property_class_identifier[] = "metternich::domain_attribute*";
	static constexpr const char database_folder[] = "domain_attributes";

	explicit domain_attribute(const std::string &identifier);
	~domain_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	bool is_taxable() const
	{
		return this->taxable;
	}

	const modifier<const domain> *get_value_modifier(const int value) const
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
	bool taxable = false;
	std::map<int, std::unique_ptr<modifier<const domain>>> value_modifiers; //the domain modifiers applied for each value; these are cumulative
};

}
