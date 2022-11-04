#pragma once

namespace metternich {

class civilian_unit_type;
class country;
class icon;
class improvement;
class tile;

class civilian_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::civilian_unit_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::country* owner READ get_owner_unconst CONSTANT)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_tile_pos_changed)
	Q_PROPERTY(bool working READ is_working NOTIFY task_completion_turns_changed)

public:
	explicit civilian_unit(const civilian_unit_type *type, const metternich::country *owner);

	const civilian_unit_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	civilian_unit_type *get_type_unconst() const
	{
		return const_cast<civilian_unit_type *>(this->get_type());
	}

public:
	void set_type(const civilian_unit_type *type)
	{
		if (type == this->get_type()) {
			return;
		}

		this->type = type;
		emit type_changed();
	}

	const icon *get_icon() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_icon_unconst() const
	{
		return const_cast<icon *>(this->get_icon());
	}

public:
	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	country *get_owner_unconst() const
	{
		return const_cast<country *>(this->get_owner());
	}

public:
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

	bool can_build_improvement(const improvement *improvement) const;
	void build_improvement(const improvement *improvement);
	void cancel_work();

	const improvement *get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const;

	void set_task_completion_turns(const int turns)
	{
		if (turns == this->task_completion_turns) {
			return;
		}

		this->task_completion_turns = turns;
		emit task_completion_turns_changed();
	}

	void do_turn();
	void disband();

signals:
	void type_changed();
	void icon_changed();
	void tile_pos_changed();
	void original_tile_pos_changed();
	void task_completion_turns_changed();

private:
	const civilian_unit_type *type = nullptr;
	const country *owner = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	QPoint original_tile_pos = QPoint(-1, -1); //the tile position before moving
	const improvement *improvement_under_construction = nullptr;
	int task_completion_turns = 0; //the remaining turns for the current task to be complete
};

}
