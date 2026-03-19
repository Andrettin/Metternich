#pragma once

#include "game/combat_base.h"
#include "script/context.h"
#include "util/dice.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("unit/military_unit.h")

namespace metternich {

class army;
class battle;
class domain;
class military_unit;

struct battle_tile final : public combat_tile_base
{
	explicit battle_tile(const terrain_type *base_terrain, const terrain_type *terrain);

	virtual bool is_occupied() const override
	{
		return this->unit != nullptr;
	}

	military_unit *unit = nullptr;
};

class battle_unit_info final : public combat_unit_info_base
{
	Q_OBJECT

	Q_PROPERTY(const metternich::military_unit* unit READ get_unit CONSTANT)

public:
	explicit battle_unit_info(const military_unit *unit, const bool defender);

	const military_unit *get_unit() const
	{
		return this->unit;
	}

	virtual const icon *get_icon() const override;
	virtual int get_hit_points() const override;
	virtual int get_max_hit_points() const override;

signals:
	void pos_changed();
	void remaining_movement_changed();

private:
	const military_unit *unit = nullptr;
};

class battle final : public combat_base
{
	Q_OBJECT

public:
	struct result final
	{
		bool attacker_victory = false;
	};

	explicit battle(army *attacking_army, army *defending_army, const QSize &map_size);
	~battle();

	virtual int get_max_range_of_units() const override;

	void set_scope(const domain *scope)
	{
		this->scope = scope;
	}

	void set_context(const context &ctx)
	{
		this->ctx = ctx;
	}

	virtual QVariantList get_unit_infos_qvariant_list() const override;
	battle_unit_info *get_unit_info(const military_unit *unit) const;
	void remove_unit_info(const military_unit *unit);

	void initialize();
	void deploy_units(std::vector<military_unit *> units, const bool defenders);

	Q_INVOKABLE QCoro::QmlTask start()
	{
		return this->start_coro();
	}

	[[nodiscard]]
	QCoro::Task<void> start_coro();

	[[nodiscard]]
	QCoro::Task<void> do_round();

	[[nodiscard]]
	QCoro::Task<void> do_unit_round(military_unit *unit, std::vector<military_unit *> &killed_units);

	const military_unit *choose_enemy(const military_unit *unit, const std::vector<military_unit *> &enemies) const;

	[[nodiscard]]
	QCoro::Task<void> do_unit_attack(const military_unit *unit, military_unit *enemy, army *enemy_army, std::vector<military_unit *> &killed_units);

	void process_result();

	virtual battle_tile &get_tile(const QPoint &tile_pos) override;
	virtual const battle_tile &get_tile(const QPoint &tile_pos) const override;
	virtual std::string get_tile_text(const QPoint &tile_pos) const override;

	virtual bool is_attacker_defeated() const override;
	virtual bool is_defender_defeated() const override;

	[[nodiscard]]
	QCoro::Task<void> move_unit_to(military_unit *unit, const QPoint tile_pos);

	virtual bool is_current_unit_in_enemy_range_at(const QPoint &tile_pos) const override;

	const site *get_location() const;

private:
	army *attacking_army = nullptr;
	army *defending_army = nullptr;
	battle::result result;
	const domain *scope = nullptr;
	context ctx;
	std::vector<battle_tile> tiles;
	std::map<const military_unit *, qunique_ptr<battle_unit_info>> unit_infos;
};

}
