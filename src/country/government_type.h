#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class technology;

template <typename scope_type>
class modifier;

class government_type final : public named_data_entry, public data_type<government_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "government_type";
	static constexpr const char property_class_identifier[] = "metternich::government_type*";
	static constexpr const char database_folder[] = "government_types";

	explicit government_type(const std::string &identifier);
	~government_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string() const;

signals:
	void changed();

private:
	technology *required_technology = nullptr;
	std::unique_ptr<const modifier<const country>> modifier;
};

}
