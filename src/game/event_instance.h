#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "script/context.h"

Q_MOC_INCLUDE("game/event.h")

namespace metternich {

class event;

class event_instance final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::event* event READ get_event CONSTANT)
	Q_PROPERTY(QString name READ get_name CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)
	Q_PROPERTY(QStringList option_names READ get_option_names CONSTANT)
	Q_PROPERTY(QStringList option_tooltips READ get_option_tooltips CONSTANT)

public:
	explicit event_instance(const metternich::event *event, const QString &name, const QString &description, const context &ctx);

	const metternich::event *get_event() const
	{
		return this->event;
	}

	const QString &get_name() const
	{
		return this->name;
	}

	const QString &get_description() const
	{
		return this->description;
	}

	const QStringList &get_option_names() const
	{
		return this->option_names;
	}

	const QStringList &get_option_tooltips() const
	{
		return this->option_tooltips;
	}

	Q_INVOKABLE void choose_option(const int displayed_option_index);

private:
	const metternich::event *event = nullptr;
	QString name;
	QString description;
	std::vector<int> option_indexes;
	QStringList option_names;
	QStringList option_tooltips;
	context ctx;
};

}
