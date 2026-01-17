#pragma once

#include "database/data_entry_container.h"

#pragma warning(push, 0)
#include <QAbstractItemModel> 
#pragma warning(pop)

namespace archimedes {
	class named_data_entry;
}

namespace metternich {

class item;

class character_data_model : public QAbstractItemModel
{
	Q_OBJECT

	Q_PROPERTY(const metternich::character* character READ get_character WRITE set_character NOTIFY character_changed)

public:
	enum role
	{
		tooltip = Qt::UserRole,
		item
	};

	struct character_data_row final
	{
		explicit character_data_row(const std::string &name, const std::string &value = "", const character_data_row *parent_row = nullptr)
			: name(name), value(value), parent_row(parent_row)
		{
		}

		std::string name;
		std::string value;
		const character_data_row *parent_row = nullptr;
		std::vector<std::unique_ptr<character_data_row>> child_rows;
		std::string tooltip;
		metternich::item *item = nullptr;
	};

	character_data_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final;
	virtual QModelIndex parent(const QModelIndex &index) const override final;

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names = QAbstractItemModel::roleNames();

		role_names.insert(static_cast<int>(role::tooltip), "tooltip");
		role_names.insert(static_cast<int>(role::item), "item");

		return role_names;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	void set_character(const metternich::character *character);

	void reset_model();
	void create_attribute_rows();
	void create_armor_class_rows();
	void update_armor_class_rows();
	void create_to_hit_bonus_rows();
	void update_to_hit_bonus_rows();
	void create_damage_row();
	void update_damage_row();
	void create_range_row();
	void update_range_row();
	void create_movement_row();
	void update_movement_row();
	void create_saving_throw_rows();
	void update_saving_throw_rows();
	void create_skill_rows();
	void update_skill_rows();
	void create_trait_rows();
	void update_trait_rows();
	void create_item_rows();
	void create_equipment_rows();
	void create_inventory_rows();

	std::optional<size_t> get_top_row_index(const character_data_row *row) const;
	void clear_child_rows(character_data_row *row);
	void on_child_rows_inserted(character_data_row *row);

signals:
	void character_changed();

private:
	const metternich::character *character = nullptr;
	std::vector<std::unique_ptr<const character_data_row>> top_rows;
	character_data_row *armor_class_row = nullptr;
	character_data_row *to_hit_bonus_row = nullptr;
	character_data_row *damage_row = nullptr;
	character_data_row *range_row = nullptr;
	character_data_row *movement_row = nullptr;
	character_data_row *saving_throw_row = nullptr;
	character_data_row *skill_row = nullptr;
	character_data_row *trait_row = nullptr;
	character_data_row *equipment_row = nullptr;
	character_data_row *inventory_row = nullptr;
	bool resetting_model = false;
};

}
