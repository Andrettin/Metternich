#include "metternich.h"

#include "character/character_data_model.h"

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "country/culture.h"
#include "religion/deity.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
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
		switch (role) {
			case Qt::DisplayRole:
			{
				const character_data_row *row_data = reinterpret_cast<const character_data_row *>(index.constInternalPointer());

				if (!row_data->value.empty()) {
					return QString::fromStdString(std::format("{} {}", row_data->name, row_data->value));
				} else {
					return QString::fromStdString(row_data->name);
				}
			}
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
	this->beginResetModel();

	this->character = character;

	this->top_rows.clear();

	if (character != nullptr) {
		const character_game_data *character_game_data = this->get_character()->get_game_data();

		this->top_rows.push_back(std::make_unique<character_data_row>("Species:", character->get_species()->get_name()));

		const character_class *character_class = character_game_data->get_character_class();
		if (character_class != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Class:", character_class->get_name()));
		}

		const int level = character_game_data->get_level();
		this->top_rows.push_back(std::make_unique<character_data_row>("Level:", std::to_string(level)));

		if (character_class != nullptr && character_game_data->get_level() < character_class->get_max_level()) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Experience:", std::format("{}/{}", number::to_formatted_string(character_game_data->get_experience()), number::to_formatted_string(character_game_data->get_experience_for_level(level + 1)))));
		} else {
			this->top_rows.push_back(std::make_unique<character_data_row>("Experience:", number::to_formatted_string(character_game_data->get_experience())));
		}

		this->top_rows.push_back(std::make_unique<character_data_row>("Age:", number::to_formatted_string(character_game_data->get_age())));

		if (character->get_culture() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Culture:", character->get_culture()->get_name()));
		}

		if (character->is_deity()) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Pantheon:", character->get_deity()->get_pantheon()->get_name()));
		} else {
			this->top_rows.push_back(std::make_unique<character_data_row>("Religion:", character->get_religion()->get_name()));
		}

		this->create_attribute_rows();

		this->top_rows.push_back(std::make_unique<character_data_row>("Hit Points:", std::format("{}/{}", character_game_data->get_hit_points(), character_game_data->get_max_hit_points())));

		this->top_rows.push_back(std::make_unique<character_data_row>("Armor Class:", std::to_string(character_game_data->get_character_class()->get_armor_class())));

		this->create_to_hit_bonus_rows();

		this->top_rows.push_back(std::make_unique<character_data_row>("Damage:", character_game_data->get_damage_dice().to_string()));
	}

	this->endResetModel();
	emit character_changed();
}

void character_data_model::create_attribute_rows()
{
	auto top_row = std::make_unique<character_data_row>("Attributes");

	const character_game_data *character_game_data = this->get_character()->get_game_data();
	for (const auto &[attribute, value] : character_game_data->get_attribute_values()) {
		auto row = std::make_unique<character_data_row>(attribute->get_name(), std::to_string(value), top_row.get());
		top_row->child_rows.push_back(std::move(row));
	}

	this->top_rows.push_back(std::move(top_row));
}

void character_data_model::create_to_hit_bonus_rows()
{
	const character_game_data *character_game_data = this->get_character()->get_game_data();

	auto to_hit_bonus_row = std::make_unique<character_data_row>("To Hit Bonus:", number::to_signed_string(character_game_data->get_to_hit_bonus()));

	this->top_rows.push_back(std::move(to_hit_bonus_row));
}

}
