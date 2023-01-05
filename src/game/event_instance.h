#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class event;

class event_instance final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)

public:
	explicit event_instance(const metternich::event *event, const QString &name, const QString &description)
		: event(event), name(name), description(description)
	{
	}

	const QString &get_name() const
	{
		return this->name;
	}

	const QString &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void choose_option(const int option_index);

private:
	const metternich::event *event = nullptr;
	QString name;
	QString description;
};

}
