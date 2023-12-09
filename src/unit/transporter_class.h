#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "unit/transporter_class_container.h"

namespace metternich {

class transporter_type;
enum class transporter_category;

class transporter_class final : public named_data_entry, public data_type<transporter_class>
{
	Q_OBJECT

	Q_PROPERTY(metternich::transporter_category category MEMBER category READ get_category)

public:
	static constexpr const char class_identifier[] = "transporter_class";
	static constexpr const char property_class_identifier[] = "metternich::transporter_class*";
	static constexpr const char database_folder[] = "transporter_classes";

public:
	static void propagate_names(const transporter_class_map<std::unique_ptr<name_generator>> &name_generators, std::unique_ptr<name_generator> &ship_name_generator);

	explicit transporter_class(const std::string &identifier);

	virtual void check() const override;

	transporter_category get_category() const
	{
		return this->category;
	}

	bool is_ship() const;

	const transporter_type *get_default_transporter_type() const
	{
		return this->default_transporter_type;
	}

	void set_default_transporter_type(const transporter_type *transporter_type);

	const std::vector<const transporter_type *> &get_transporter_types() const
	{
		return this->transporter_types;
	}

	void add_transporter_type(const transporter_type *transporter_type)
	{
		this->transporter_types.push_back(transporter_type);
	}

private:
	transporter_category category;
	const transporter_type *default_transporter_type = nullptr;
	std::vector<const transporter_type *> transporter_types;
};

}
