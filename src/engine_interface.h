#pragma once

#include "util/qunique_ptr.h"
#include "util/singleton.h"

namespace metternich {

class defines;
class event_instance;
class game;
class map;
class preferences;

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(double scale_factor READ get_scale_factor NOTIFY scale_factor_changed)
	Q_PROPERTY(metternich::defines* defines READ get_defines CONSTANT)
	Q_PROPERTY(metternich::game* game READ get_game CONSTANT)
	Q_PROPERTY(metternich::map* map READ get_map CONSTANT)
	Q_PROPERTY(metternich::preferences* preferences READ get_preferences CONSTANT)

public:
	engine_interface();
	engine_interface(const engine_interface &other) = delete;
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

	double get_scale_factor() const;

	defines *get_defines() const;
	game *get_game() const;
	map *get_map() const;
	preferences *get_preferences() const;

	Q_INVOKABLE QObject *get_map_template(const QString &identifier) const;
	Q_INVOKABLE QVariantList get_scenarios() const;
	Q_INVOKABLE QVariantList get_eras() const;

	void add_event_instance(qunique_ptr<event_instance> &&event_instance);
	void remove_event_instance(event_instance *event_instance);

	engine_interface &operator =(const engine_interface &other) = delete;

signals:
	void running_changed();
	void scale_factor_changed();
	void event_fired(QObject *event_instance);
	void event_closed(QObject *event_instance);

private:
	bool running = false;
	std::vector<qunique_ptr<event_instance>> event_instances;
};

}
