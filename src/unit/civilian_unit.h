#pragma once

#include "economy/resource_container.h"
#include "map/terrain_type_container.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/civilian_unit_type.h")

namespace metternich {

class character;
class civilian_unit_type;
class country;
class culture;
class icon;
class improvement;
class phenotype;
class population_type;
class religion;
class site;
class tile;

class civilian_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::civilian_unit_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::country* owner READ get_owner CONSTANT)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)
	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_tile_pos_changed)
	Q_PROPERTY(bool working READ is_working NOTIFY task_completion_turns_changed)
	Q_PROPERTY(QVariantList improvable_resource_tiles READ get_improvable_resource_tiles_qvariant_list NOTIFY improvable_resources_changed)
	Q_PROPERTY(QVariantList prospectable_tiles READ get_prospectable_tiles_qvariant_list NOTIFY prospectable_tiles_changed)

public:
	static constexpr int improvement_construction_turns = 2;
	static constexpr int exploration_turns = 1;
	static constexpr int prospection_turns = 1;

	explicit civilian_unit(const civilian_unit_type *type, const country *owner, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const site *home_settlement);
	explicit civilian_unit(const civilian_unit_type *type, const country *owner, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const site *home_settlement);
	explicit civilian_unit(const metternich::character *character, const country *owner);

	void do_turn();
	void do_ai_turn();

	const civilian_unit_type *get_type() const
	{
		return this->type;
	}

	void set_type(const civilian_unit_type *type)
	{
		if (type == this->get_type()) {
			return;
		}

		this->type = type;
		emit type_changed();
	}

	const icon *get_icon() const;

	const country *get_owner() const
	{
		return this->owner;
	}

	const metternich::population_type *get_population_type() const
	{
		return this->population_type;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const site *get_home_settlement() const
	{
		return this->home_settlement;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos);
	tile *get_tile() const;

	void set_original_tile_pos(const QPoint &tile_pos)
	{
		if (tile_pos == this->original_tile_pos) {
			return;
		}

		this->original_tile_pos = tile_pos;
		emit original_tile_pos_changed();
	}

	Q_INVOKABLE bool can_move_to(const QPoint &tile_pos) const;
	Q_INVOKABLE void move_to(const QPoint &tile_pos);
	Q_INVOKABLE void cancel_move();

	bool is_moving() const
	{
		return this->original_tile_pos != QPoint(-1, -1);
	}

	bool is_working() const
	{
		return this->task_completion_turns > 0;
	}

	bool is_busy() const
	{
		return this->is_moving() || this->is_working();
	}

	Q_INVOKABLE bool can_build_on_tile() const;
	Q_INVOKABLE void build_on_tile();

	bool can_build_improvement(const improvement *improvement) const;
	bool can_build_improvement_on_tile(const improvement *improvement, const QPoint &tile_pos) const;
	void build_improvement(const improvement *improvement);
	Q_INVOKABLE void cancel_work();

	const improvement *get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const;

	resource_map<std::vector<QPoint>> get_improvable_resource_tiles() const;
	QVariantList get_improvable_resource_tiles_qvariant_list() const;

	bool can_explore_tile(const QPoint &tile_pos) const;

	bool can_prospect_tile(const QPoint &tile_pos) const;
	terrain_type_map<std::vector<QPoint>> get_prospectable_tiles() const;
	QVariantList get_prospectable_tiles_qvariant_list() const;

	void set_task_completion_turns(const int turns)
	{
		if (turns == this->task_completion_turns) {
			return;
		}

		this->task_completion_turns = turns;
		emit task_completion_turns_changed();
	}

	void disband(const bool dead);
	Q_INVOKABLE void disband();

signals:
	void type_changed();
	void icon_changed();
	void tile_pos_changed();
	void original_tile_pos_changed();
	void improvable_resources_changed();
	void prospectable_tiles_changed();
	void task_completion_turns_changed();

private:
	const civilian_unit_type *type = nullptr;
	const country *owner = nullptr;
	const metternich::population_type *population_type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const site *home_settlement = nullptr;
	const metternich::character *character = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	QPoint original_tile_pos = QPoint(-1, -1); //the tile position before moving
	const improvement *improvement_under_construction = nullptr;
	bool exploring = false;
	bool prospecting = false;
	int task_completion_turns = 0; //the remaining turns for the current task to be complete
};

}
