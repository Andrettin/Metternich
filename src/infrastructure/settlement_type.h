#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class site;

template <typename scope_type>
class condition;

class settlement_type final : public named_data_entry, public data_type<settlement_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "settlement_type";
	static constexpr const char property_class_identifier[] = "metternich::settlement_type*";
	static constexpr const char database_folder[] = "settlement_types";

	explicit settlement_type(const std::string &identifier);
	~settlement_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::vector<const settlement_type *> &get_base_settlement_types() const
	{
		return this->base_settlement_types;
	}

	const std::vector<const settlement_type *> &get_upgraded_settlement_types() const
	{
		return this->upgraded_settlement_types;
	}

	const condition<site> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	std::vector<const settlement_type *> base_settlement_types;
	std::vector<const settlement_type *> upgraded_settlement_types;
	std::unique_ptr<const condition<site>> conditions;
};

}
