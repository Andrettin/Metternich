#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "script/context.h"

namespace metternich {

class event;

class event_instance final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)
	Q_PROPERTY(QStringList options READ get_options CONSTANT)
	Q_PROPERTY(QStringList option_tooltips READ get_option_tooltips CONSTANT)

public:
	explicit event_instance(const metternich::event *event, const QString &name, const QString &description, const context &ctx);

	const QString &get_name() const
	{
		return this->name;
	}

	const QString &get_description() const
	{
		return this->description;
	}

	const QStringList &get_options() const
	{
		return this->options;
	}

	const QStringList &get_option_tooltips() const
	{
		return this->option_tooltips;
	}

	Q_INVOKABLE void choose_option(const int option_index);

private:
	const metternich::event *event = nullptr;
	QString name;
	QString description;
	QStringList options;
	QStringList option_tooltips;
	context ctx;
};

}
