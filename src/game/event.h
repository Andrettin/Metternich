#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country;

template <typename scope_type>
class condition;

class event final : public named_data_entry, public data_type<event>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "event";
	static constexpr const char property_class_identifier[] = "metternich::event*";
	static constexpr const char database_folder[] = "events";

	explicit event(const std::string &identifier);
	~event();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	void fire(const country *country) const;

signals:
	void changed();

private:
	std::string description;
	std::unique_ptr<const condition<country>> conditions;
};

}
