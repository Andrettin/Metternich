#pragma once

Q_MOC_INCLUDE("spell/spell.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class spell;
class terrain_type;

enum class combat_placement
{
	left,
	center,
	right
};

struct combat_tile_base
{
	explicit combat_tile_base(const terrain_type *base_terrain, const terrain_type *terrain);

	virtual bool is_occupied() const = 0;

	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	std::array<short, 4> base_subtile_frames{};
	short tile_frame = 0;
	std::array<short, 4> subtile_frames{};
};

class combat_unit_info_base : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY pos_changed)
	Q_PROPERTY(QPoint pixel_offset READ get_pixel_offset NOTIFY pos_changed)
	Q_PROPERTY(bool defender READ is_defender CONSTANT)
	Q_PROPERTY(int remaining_movement READ get_remaining_movement NOTIFY remaining_movement_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(int hit_points READ get_hit_points NOTIFY hit_points_changed)
	Q_PROPERTY(int max_hit_points READ get_max_hit_points NOTIFY max_hit_points_changed)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)

public:
	explicit combat_unit_info_base(const bool defender)
		: defender(defender), placement(defender ? combat_placement::right : combat_placement::left)
	{
	}

	combat_placement get_placement() const
	{
		return this->placement;
	}

	void set_placement(const combat_placement placement)
	{
		this->placement = placement;
	}

	const QPoint &get_placement_offset() const
	{
		return this->placement_offset;
	}

	void set_placement_offset(const QPoint &offset)
	{
		this->placement_offset = offset;
	}

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos)
	{
		if (tile_pos == this->get_tile_pos()) {
			return;
		}

		this->tile_pos = tile_pos;
		this->pixel_offset = QPoint(0, 0);
		emit pos_changed();
	}

	const QPoint &get_pixel_offset() const
	{
		return this->pixel_offset;
	}

	void set_pixel_offset(const QPoint &pixel_offset)
	{
		if (pixel_offset == this->get_pixel_offset()) {
			return;
		}

		this->pixel_offset = pixel_offset;
		emit pos_changed();
	}

	bool is_defender() const
	{
		return this->defender;
	}

	int get_remaining_movement() const
	{
		return this->remaining_movement;
	}

	void set_remaining_movement(const int movement)
	{
		if (movement == this->get_remaining_movement()) {
			return;
		}

		this->remaining_movement = movement;
	}

	void change_remaining_movement(const int change)
	{
		this->set_remaining_movement(this->get_remaining_movement() + change);
	}

	virtual const icon *get_icon() const = 0;
	virtual int get_hit_points() const = 0;
	virtual int get_max_hit_points() const = 0;
	virtual const character *get_character() const = 0;

signals:
	void pos_changed();
	void remaining_movement_changed();
	void icon_changed();
	void hit_points_changed();
	void max_hit_points_changed();

private:
	combat_placement placement = combat_placement::right;
	QPoint placement_offset = QPoint(0, 0);
	QPoint tile_pos;
	QPoint pixel_offset = QPoint(0, 0);
	bool defender = false;
	int remaining_movement = 0;
};

class combat_base : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList unit_infos READ get_unit_infos_qvariant_list NOTIFY unit_infos_changed)
	Q_PROPERTY(const metternich::combat_unit_info_base* current_unit READ get_current_unit NOTIFY current_unit_changed)
	Q_PROPERTY(const metternich::spell* current_spell READ get_current_spell WRITE set_current_spell NOTIFY current_spell_changed)
	Q_PROPERTY(bool autoplay_enabled READ is_autoplay_enabled WRITE set_autoplay_enabled NOTIFY autoplay_enabled_changed)

public:
	combat_base();

	void set_map_size(const QSize &map_size);

	const QRect &get_map_rect() const
	{
		return this->map_rect;
	}

	int get_map_width() const
	{
		return this->get_map_rect().width();
	}

	int get_map_height() const
	{
		return this->get_map_rect().height();
	}

	const terrain_type *get_base_terrain() const
	{
		return this->base_terrain;
	}

	void set_base_terrain(const terrain_type *terrain);

	void set_attacker_retreat_allowed(const bool allowed)
	{
		this->attacker_retreat_allowed = allowed;
	}

	void set_defender_retreat_allowed(const bool allowed)
	{
		this->defender_retreat_allowed = allowed;
	}

	virtual QVariantList get_unit_infos_qvariant_list() const = 0;
	virtual int get_max_range_of_units() const = 0;

	virtual combat_tile_base &get_tile(const QPoint &tile_pos) = 0;
	virtual const combat_tile_base &get_tile(const QPoint &tile_pos) const = 0;
	virtual std::string get_tile_text(const QPoint &tile_pos) const;
	bool is_tile_attacker_escape(const QPoint &tile_pos) const;
	bool is_tile_defender_escape(const QPoint &tile_pos) const;

	virtual bool is_attacker_defeated() const = 0;
	virtual bool is_defender_defeated() const = 0;

	[[nodiscard]]
	QCoro::Task<QPoint> get_target();

	Q_INVOKABLE void set_target(const QPoint &tile_pos);

	const combat_unit_info_base *get_current_unit() const
	{
		return this->current_unit;
	}

	void set_current_unit(const combat_unit_info_base *unit)
	{
		this->current_unit = unit;

		emit current_unit_changed();
	}

	bool can_current_unit_move_to(const QPoint &tile_pos) const;
	bool can_current_unit_retreat_at(const QPoint &tile_pos) const;
	virtual bool is_current_unit_in_enemy_range_at(const QPoint &tile_pos) const = 0;

	const spell *get_current_spell() const
	{
		return this->current_spell;
	}

	void set_current_spell(const spell *spell)
	{
		this->current_spell = spell;

		emit current_spell_changed();
	}

	bool is_autoplay_enabled() const
	{
		return this->autoplay_enabled;
	}

	void set_autoplay_enabled(const bool enabled)
	{
		this->autoplay_enabled = enabled;

		this->set_current_spell(nullptr); //autoplay can't handle spells yet

		emit autoplay_enabled_changed();
	}

	const std::unique_ptr<QPromise<bool>> &get_promise() const
	{
		return this->promise;
	}

	QFuture<bool> get_future() const
	{
		return this->promise->future();
	}

	Q_INVOKABLE void clear();

signals:
	void unit_infos_changed();
	void tile_unit_changed(const QPoint &tile_pos);
	void tile_object_changed(const QPoint &tile_pos);
	void current_unit_changed();
	void current_spell_changed();
	void movable_tiles_changed();
	void autoplay_enabled_changed();
	void finished();

private:
	QRect map_rect;
	const terrain_type *base_terrain = nullptr;
	bool attacker_retreat_allowed = true;
	bool defender_retreat_allowed = true;
	std::unique_ptr<QPromise<QPoint>> target_promise;
	const combat_unit_info_base *current_unit = nullptr;
	const spell *current_spell = nullptr;
	bool autoplay_enabled = false;
	std::unique_ptr<QPromise<bool>> promise;
};

}
