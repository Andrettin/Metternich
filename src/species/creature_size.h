#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

template <typename scope_type>
class modifier;

class creature_size final : public named_data_entry, public data_type<creature_size>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "creature_size";
	static constexpr const char property_class_identifier[] = "metternich::creature_size*";
	static constexpr const char database_folder[] = "creature_sizes";

	static void initialize_all();

	explicit creature_size(const std::string &identifier);
	~creature_size();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	int get_min_dimension() const
	{
		return this->min_dimension;
	}

	int get_max_dimension() const
	{
		return this->max_dimension;
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

private:
	int min_dimension = 0; //in inches
	int max_dimension = 0; //in inches
	std::unique_ptr<const metternich::modifier<const character>> modifier;
};

}
