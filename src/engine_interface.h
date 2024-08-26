#pragma once

#include "util/qunique_ptr.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("database/defines.h")
Q_MOC_INCLUDE("database/preferences.h")
Q_MOC_INCLUDE("game/event_instance.h")
Q_MOC_INCLUDE("game/game.h")
Q_MOC_INCLUDE("map/map.h")
Q_MOC_INCLUDE("map/map_template.h")
Q_MOC_INCLUDE("map/world.h")

namespace metternich {

class consulate;
class country_tier_data;
class defines;
class event_instance;
class game;
class map;
class map_template;
class military_unit;
class preferences;
class province;
class world;
enum class country_tier;
enum class military_unit_category;

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
	Q_PROPERTY(QVariantList selected_military_units READ get_selected_military_units_qvariant_list NOTIFY selected_military_units_changed)

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

	Q_INVOKABLE const map_template *get_map_template(const QString &identifier) const;
	Q_INVOKABLE const world *get_world(const QString &identifier) const;
	Q_INVOKABLE QVariantList get_scenarios() const;
	Q_INVOKABLE QVariantList get_major_scenarios() const;
	Q_INVOKABLE QVariantList get_eras() const;
	Q_INVOKABLE QVariantList get_law_groups() const;
	Q_INVOKABLE QVariantList get_policies() const;
	Q_INVOKABLE QVariantList get_technologies() const;
	Q_INVOKABLE const metternich::country_tier_data *get_country_tier_data(const metternich::country_tier tier) const;
	Q_INVOKABLE const consulate *get_consulate(const QString &identifier) const;

	void add_notification(const QString &title, const QObject *portrait_object, const QString &text)
	{
		emit notification_added(title, portrait_object, text);
	}

	void add_notification(const std::string &title, const QObject *portrait_object, const std::string &text)
	{
		this->add_notification(QString::fromStdString(title), portrait_object, QString::fromStdString(text));
	}

	void add_event_instance(qunique_ptr<event_instance> &&event_instance);
	void remove_event_instance(event_instance *event_instance);

	const std::vector<military_unit *> &get_selected_military_units() const
	{
		return this->selected_military_units;
	}

	QVariantList get_selected_military_units_qvariant_list() const;

	void add_selected_military_unit(military_unit *military_unit)
	{
		this->selected_military_units.push_back(military_unit);
		emit selected_military_units_changed();
	}

	void remove_selected_military_unit(const military_unit *military_unit)
	{
		std::erase(this->selected_military_units, military_unit);
		emit selected_military_units_changed();
	}

	Q_INVOKABLE void clear_selected_military_units()
	{
		this->selected_military_units.clear();
		emit selected_military_units_changed();
	}

	Q_INVOKABLE int get_selected_military_unit_category_count(const metternich::military_unit_category category);
	Q_INVOKABLE void change_selected_military_unit_category_count(const metternich::military_unit_category category, const int change, metternich::province *province);

	Q_INVOKABLE void move_selected_military_units_to(const QPoint &tile_pos);

	engine_interface &operator =(const engine_interface &other) = delete;

signals:
	void running_changed();
	void scale_factor_changed();
	void notification_added(const QString &title, const QObject *portrait_object, const QString &text);
	void event_fired(const event_instance *event_instance);
	void event_closed(const event_instance *event_instance);
	void current_research_choosable(const QVariantList &potential_technologies);
	void free_technology_choosable(const QVariantList &potential_technologies);
	void next_tradition_choosable(const QVariantList &potential_traditions);
	void next_belief_choosable(const QVariantList &potential_beliefs);
	void next_advisor_choosable(const QVariantList &potential_advisors);
	void next_leader_choosable(const QVariantList &potential_leaders);
	void selected_military_units_changed();

private:
	bool running = false;
	std::vector<qunique_ptr<event_instance>> event_instances;
	std::vector<military_unit *> selected_military_units;
};

}
