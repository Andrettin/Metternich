#pragma once

#include "util/singleton.h"

namespace metternich {

class defines;
class map;

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(metternich::defines* defines READ get_defines CONSTANT)
	Q_PROPERTY(metternich::map* map READ get_map CONSTANT)

public:
	engine_interface();
	~engine_interface();

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	defines *get_defines() const;
	map *get_map() const;

	Q_INVOKABLE QObject *get_map_template(const QString &identifier) const;

signals:
	void running_changed();

private:
	bool running = false;
};

}
