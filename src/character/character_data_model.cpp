#include "metternich.h"

#include "character/character_data_model.h"

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/saving_throw_type.h"
#include "domain/culture.h"
#include "item/item.h"
#include "item/item_slot.h"
#include "religion/deity.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace metternich {

character_data_model::character_data_model()
{
}

int character_data_model::rowCount(const QModelIndex &parent) const
{
	if (this->get_character() == nullptr) {
		return 0;
	}

	if (!parent.isValid()) {
		return static_cast<int>(this->top_rows.size());
	}

	const character_data_row *parent_row_data = reinterpret_cast<const character_data_row *>(parent.constInternalPointer());
	return static_cast<int>(parent_row_data->child_rows.size());
}

int character_data_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant character_data_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const character_data_row *row_data = reinterpret_cast<const character_data_row *>(index.constInternalPointer());

		switch (role) {
			case Qt::DisplayRole:
				if (!row_data->value.empty()) {
					return QString::fromStdString(std::format("{} {}", row_data->name, row_data->value));
				} else {
					return QString::fromStdString(row_data->name);
				}
			case role::item:
				return QVariant::fromValue(row_data->item);
			default:
				throw std::runtime_error(std::format("Invalid character data model role: {}.", role));
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

QModelIndex character_data_model::index(const int row, const int column, const QModelIndex &parent) const
{
	if (!this->hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (!parent.isValid()) {
		return this->createIndex(row, column, this->top_rows.at(row).get());
	}

	const character_data_row *parent_row_data = reinterpret_cast<const character_data_row *>(parent.constInternalPointer());
	return this->createIndex(row, column, parent_row_data->child_rows.at(row).get());
}

QModelIndex character_data_model::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	const character_data_row *row_data = reinterpret_cast<const character_data_row *>(index.constInternalPointer());
	if (row_data->parent_row == nullptr) {
		return QModelIndex();
	}

	if (row_data->parent_row->parent_row == nullptr) {
		for (size_t i = 0; i < this->top_rows.size(); ++i) {
			if (this->top_rows.at(i).get() == row_data->parent_row) {
				return this->createIndex(static_cast<int>(i), 0, row_data->parent_row);
			}
		}

		assert_throw(false);
	}

	for (size_t i = 0; i < row_data->parent_row->parent_row->child_rows.size(); ++i) {
		if (row_data->parent_row->parent_row->child_rows.at(i).get() == row_data->parent_row) {
			return this->createIndex(static_cast<int>(i), 0, row_data->parent_row);
		}
	}
	assert_throw(false);

	return QModelIndex();
}

void character_data_model::set_character(const metternich::character *character)
{
	if (this->character != nullptr) {
		disconnect(this->character->get_game_data(), &character_game_data::armor_class_bonus_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		disconnect(this->character->get_game_data(), &character_game_data::saving_throw_bonuses_changed, this, &character_data_model::update_saving_throw_rows);
		disconnect(this->character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::update_damage_row);
		disconnect(this->character->get_game_data(), &character_game_data::items_changed, this, &character_data_model::create_inventory_rows);
		disconnect(this->character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::create_item_rows);
	}

	this->character = character;

	this->reset_model();

	if (character != nullptr) {
		connect(this->character->get_game_data(), &character_game_data::armor_class_bonus_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		connect(this->character->get_game_data(), &character_game_data::saving_throw_bonuses_changed, this, &character_data_model::update_saving_throw_rows);
		connect(this->character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::update_damage_row);
		connect(character->get_game_data(), &character_game_data::items_changed, this, &character_data_model::create_inventory_rows);
		connect(character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::create_item_rows);
	}

	emit character_changed();
}

void character_data_model::reset_model()
{
	this->beginResetModel();
	this->resetting_model = true;

	this->top_rows.clear();
	this->armor_class_row = nullptr;
	this->to_hit_bonus_row = nullptr;
	this->damage_row = nullptr;
	this->saving_throw_row = nullptr;
	this->equipment_row = nullptr;
	this->inventory_row = nullptr;

	if (this->character != nullptr) {
		const character_game_data *character_game_data = this->get_character()->get_game_data();

		this->top_rows.push_back(std::make_unique<character_data_row>("Species:", this->character->get_species()->get_name()));

		const character_class *character_class = character_game_data->get_character_class();
		if (character_class != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Class:", character_class->get_name()));

			const int level = character_game_data->get_level();
			this->top_rows.push_back(std::make_unique<character_data_row>("Level:", std::to_string(level)));

			if (character_game_data->get_level() < character_class->get_max_level()) {
				this->top_rows.push_back(std::make_unique<character_data_row>("Experience:", std::format("{}/{}", number::to_formatted_string(character_game_data->get_experience()), number::to_formatted_string(character_game_data->get_experience_for_level(level + 1)))));
			} else {
				this->top_rows.push_back(std::make_unique<character_data_row>("Experience:", number::to_formatted_string(character_game_data->get_experience())));
			}
		}

		this->top_rows.push_back(std::make_unique<character_data_row>("Age:", number::to_formatted_string(character_game_data->get_age())));

		if (this->character->get_culture() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Culture:", this->character->get_culture()->get_name()));
		}

		if (this->character->is_deity()) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Pantheon:", this->character->get_deity()->get_pantheon()->get_name()));
		} else if (this->character->get_religion() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Religion:", this->character->get_religion()->get_name()));
		}

		this->create_attribute_rows();

		this->top_rows.push_back(std::make_unique<character_data_row>("Hit Points:", std::format("{}/{}", character_game_data->get_hit_points(), character_game_data->get_max_hit_points())));

		this->create_armor_class_rows();
		this->create_to_hit_bonus_rows();
		this->create_damage_row();
		this->create_saving_throw_rows();

		if (!character_game_data->get_items().empty()) {
			this->create_item_rows();
		}
	}

	this->resetting_model = false;
	this->endResetModel();
}

void character_data_model::create_attribute_rows()
{
	auto top_row = std::make_unique<character_data_row>("Attributes");

	const character_game_data *character_game_data = this->get_character()->get_game_data();
	for (const auto &[attribute, value] : character_game_data->get_attribute_values()) {
		auto row = std::make_unique<character_data_row>(attribute->get_name() + ":", std::to_string(value), top_row.get());
		top_row->child_rows.push_back(std::move(row));
	}

	this->top_rows.push_back(std::move(top_row));
}


void character_data_model::create_armor_class_rows()
{
	auto row = std::make_unique<character_data_row>("Armor Class:");
	this->armor_class_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_armor_class_rows();
}

void character_data_model::update_armor_class_rows()
{
	assert_throw(this->armor_class_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->armor_class_row->value = std::to_string(character_game_data->get_armor_class_bonus());

	this->clear_child_rows(this->armor_class_row);

	for (const auto &[species, bonus] : character_game_data->get_species_armor_class_bonuses()) {
		auto row = std::make_unique<character_data_row>(std::format("Against {}:", string::get_plural_form(species->get_name())), std::to_string(character_game_data->get_armor_class_bonus() + bonus), this->armor_class_row);
		this->armor_class_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->armor_class_row);
}

void character_data_model::create_to_hit_bonus_rows()
{
	auto row = std::make_unique<character_data_row>("To Hit Bonus:");
	this->to_hit_bonus_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_to_hit_bonus_rows();
}

void character_data_model::update_to_hit_bonus_rows()
{
	assert_throw(this->to_hit_bonus_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();
	this->to_hit_bonus_row->value = number::to_signed_string(character_game_data->get_to_hit_bonus());
}

void character_data_model::create_damage_row()
{
	auto row = std::make_unique<character_data_row>("Damage:");
	this->damage_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_damage_row();
}

void character_data_model::update_damage_row()
{
	assert_throw(this->damage_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();
	this->damage_row->value = character_game_data->get_damage_dice().to_display_string();
}

void character_data_model::create_saving_throw_rows()
{
	auto row = std::make_unique<character_data_row>("Saving Throws");
	this->saving_throw_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_saving_throw_rows();
}

void character_data_model::update_saving_throw_rows()
{
	assert_throw(this->saving_throw_row != nullptr);

	this->clear_child_rows(this->saving_throw_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	for (const auto &[saving_throw_type, bonus] : character_game_data->get_saving_throw_bonuses()) {
		auto row = std::make_unique<character_data_row>(saving_throw_type->get_name() + ":", number::to_signed_string(bonus), this->saving_throw_row);
		this->saving_throw_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->saving_throw_row);
}

void character_data_model::create_item_rows()
{
	this->create_equipment_rows();
	this->create_inventory_rows();
}

void character_data_model::create_equipment_rows()
{
	const character_game_data *character_game_data = this->get_character()->get_game_data();

	size_t equipment_row_index = 0;

	if (this->equipment_row == nullptr) {
		auto top_row = std::make_unique<character_data_row>("Equipment");
		this->equipment_row = top_row.get();

		const std::optional<size_t> inventory_row_index = this->get_top_row_index(this->inventory_row);

		if (inventory_row_index.has_value()) {
			this->top_rows.insert(this->top_rows.begin() + inventory_row_index.value(), std::move(top_row));
			equipment_row_index = inventory_row_index.value();
		} else {
			this->top_rows.push_back(std::move(top_row));
			equipment_row_index = this->top_rows.size() - 1;
		}

		if (!this->resetting_model) {
			this->beginInsertRows(QModelIndex(), static_cast<int>(equipment_row_index), static_cast<int>(equipment_row_index));
			this->endInsertRows();
		}
	}

	if (equipment_row_index == 0) {
		equipment_row_index = this->get_top_row_index(this->equipment_row).value();
	}

	this->clear_child_rows(this->equipment_row);

	for (const auto &[item_slot, count] : this->get_character()->get_species()->get_item_slot_counts()) {
		const std::vector<metternich::item *> &slot_equipped_items = character_game_data->get_equipped_items(item_slot);

		for (int i = 0; i < count; ++i) {
			if (static_cast<size_t>(i) >= slot_equipped_items.size()) {
				break;
			}

			std::string slot_name;
			if (count > 1) {
				slot_name = std::format("{} {}:", item_slot->get_name(), i + 1);
			} else {
				slot_name = item_slot->get_name() + ":";
			}

			auto row = std::make_unique<character_data_row>(slot_name, slot_equipped_items[i]->get_name(), this->equipment_row);
			row->item = slot_equipped_items[i];
			this->equipment_row->child_rows.push_back(std::move(row));
		}
	}

	if (this->equipment_row->child_rows.empty()) {
		if (!this->resetting_model) {
			this->beginRemoveRows(QModelIndex(), static_cast<int>(equipment_row_index), static_cast<int>(equipment_row_index));
		}

		this->top_rows.erase(this->top_rows.begin() + equipment_row_index);
		this->equipment_row = nullptr;

		if (!this->resetting_model) {
			this->endRemoveRows();
		}
	} else {
		this->on_child_rows_inserted(this->equipment_row);
	}
}

void character_data_model::create_inventory_rows()
{
	const character_game_data *character_game_data = this->get_character()->get_game_data();

	if (this->inventory_row == nullptr) {
		auto top_row = std::make_unique<character_data_row>("Inventory");
		this->inventory_row = top_row.get();
		this->top_rows.push_back(std::move(top_row));

		if (!this->resetting_model) {
			this->beginInsertRows(QModelIndex(), static_cast<int>(this->top_rows.size()) - 1, static_cast<int>(this->top_rows.size()) - 1);
			this->endInsertRows();
		}
	}

	this->clear_child_rows(this->inventory_row);

	for (const qunique_ptr<metternich::item> &item : character_game_data->get_items()) {
		if (item->is_equipped()) {
			continue;
		}

		auto row = std::make_unique<character_data_row>(item->get_name(), std::string(), this->inventory_row);
		row->item = item.get();
		this->inventory_row->child_rows.push_back(std::move(row));
	}

	if (this->inventory_row->child_rows.empty()) {
		const size_t inventory_row_index = this->get_top_row_index(this->inventory_row).value();

		if (!this->resetting_model) {
			this->beginRemoveRows(QModelIndex(), static_cast<int>(inventory_row_index), static_cast<int>(inventory_row_index));
		}

		this->top_rows.erase(this->top_rows.begin() + inventory_row_index);
		this->inventory_row = nullptr;

		if (!this->resetting_model) {
			this->endRemoveRows();
		}
	} else {
		this->on_child_rows_inserted(this->inventory_row);
	}
}

std::optional<size_t> character_data_model::get_top_row_index(const character_data_row *row) const
{
	if (row != nullptr) {
		for (size_t i = 0; i < this->top_rows.size(); ++i) {
			const std::unique_ptr<const character_data_row> &top_row = this->top_rows.at(i);
			if (top_row.get() == row) {
				return i;
			}
		}
	}

	return std::nullopt;
}

void character_data_model::clear_child_rows(character_data_row *row)
{
	const size_t row_index = this->get_top_row_index(row).value();
	const QModelIndex row_model_index = this->index(static_cast<int>(row_index), 0);
	const size_t child_row_count = row->child_rows.size();

	if (child_row_count == 0) {
		return;
	}

	if (!this->resetting_model) {
		this->beginRemoveRows(row_model_index, 0, static_cast<int>(child_row_count) - 1);
	}

	row->child_rows.clear();

	if (!this->resetting_model) {
		this->endRemoveRows();
	}
}

void character_data_model::on_child_rows_inserted(character_data_row *row)
{
	if (!this->resetting_model && !row->child_rows.empty()) {
		const size_t row_index = this->get_top_row_index(row).value();
		const QModelIndex row_model_index = this->index(static_cast<int>(row_index), 0);
		this->beginInsertRows(row_model_index, 0, static_cast<int>(row->child_rows.size()) - 1);
		this->endInsertRows();
	}
}

}
