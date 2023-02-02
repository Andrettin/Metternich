#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "script/context.h"

namespace metternich {

class event;

class event_instance final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::event* event READ get_event_unconst CONSTANT)
	Q_PROPERTY(QString name READ get_name CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)
	Q_PROPERTY(QStringList option_names READ get_option_names CONSTANT)
	Q_PROPERTY(QStringList option_tooltips READ get_option_tooltips CONSTANT)

public:
	explicit event_instance(const metternich::event *event, const QString &name, const QString &description, const context &ctx);

private:
	//for the Qt property (pointers there can't be const)
	metternich::event *get_event_unconst() const
	{
		return const_cast<metternich::event *>(this->event);
	}

public:
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
