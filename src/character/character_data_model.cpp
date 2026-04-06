#include "metternich.h"

#include "character/character_data_model.h"

#include "character/bloodline.h"
#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/dynasty.h"
#include "character/mythic_path.h"
#include "character/saving_throw_type.h"
#include "character/skill.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "religion/deity.h"
#include "religion/divine_domain.h"
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
			case role::tooltip:
				return QString::fromStdString(row_data->tooltip);
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
		disconnect(this->character->get_game_data(), &character_game_data::attribute_values_changed, this, &character_data_model::update_attribute_rows);
		disconnect(this->character->get_game_data(), &character_game_data::mana_changed, this, &character_data_model::update_mana_row);
		disconnect(this->character->get_game_data(), &character_game_data::max_mana_changed, this, &character_data_model::update_mana_row);
		disconnect(this->character->get_game_data(), &character_game_data::craft_changed, this, &character_data_model::update_craft_row);
		disconnect(this->character->get_game_data(), &character_game_data::max_craft_changed, this, &character_data_model::update_craft_row);
		disconnect(this->character->get_game_data(), &character_game_data::armor_class_bonus_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		disconnect(this->character->get_game_data(), &character_game_data::damage_bonus_changed, this, &character_data_model::update_damage_row);
		disconnect(this->character->get_game_data(), &character_game_data::range_changed, this, &character_data_model::update_range_row);
		disconnect(this->character->get_game_data(), &character_game_data::movement_changed, this, &character_data_model::update_movement_row);
		disconnect(this->character->get_game_data(), &character_game_data::initiative_bonus_changed, this, &character_data_model::update_initiative_bonus_row);
		disconnect(this->character->get_game_data(), &character_game_data::saving_throw_bonuses_changed, this, &character_data_model::update_saving_throw_rows);
		disconnect(this->character->get_game_data(), &character_game_data::skill_trainings_changed, this, &character_data_model::update_skill_rows);
		disconnect(this->character->get_game_data(), &character_game_data::skill_values_changed, this, &character_data_model::update_skill_rows);
		disconnect(this->character->get_game_data(), &character_game_data::traits_changed, this, &character_data_model::update_trait_rows);
		disconnect(this->character->get_game_data(), &character_game_data::wealth_changed, this, &character_data_model::update_wealth_row);
	}

	this->character = character;

	this->reset_model();

	if (character != nullptr) {
		connect(this->character->get_game_data(), &character_game_data::attribute_values_changed, this, &character_data_model::update_attribute_rows);
		connect(this->character->get_game_data(), &character_game_data::mana_changed, this, &character_data_model::update_mana_row);
		connect(this->character->get_game_data(), &character_game_data::max_mana_changed, this, &character_data_model::update_mana_row);
		connect(this->character->get_game_data(), &character_game_data::craft_changed, this, &character_data_model::update_craft_row);
		connect(this->character->get_game_data(), &character_game_data::max_craft_changed, this, &character_data_model::update_craft_row);
		connect(this->character->get_game_data(), &character_game_data::armor_class_bonus_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		connect(this->character->get_game_data(), &character_game_data::damage_bonus_changed, this, &character_data_model::update_damage_row);
		connect(this->character->get_game_data(), &character_game_data::range_changed, this, &character_data_model::update_range_row);
		connect(this->character->get_game_data(), &character_game_data::movement_changed, this, &character_data_model::update_movement_row);
		connect(this->character->get_game_data(), &character_game_data::initiative_bonus_changed, this, &character_data_model::update_initiative_bonus_row);
		connect(this->character->get_game_data(), &character_game_data::saving_throw_bonuses_changed, this, &character_data_model::update_saving_throw_rows);
		connect(this->character->get_game_data(), &character_game_data::skill_trainings_changed, this, &character_data_model::update_skill_rows);
		connect(this->character->get_game_data(), &character_game_data::skill_values_changed, this, &character_data_model::update_skill_rows);
		connect(this->character->get_game_data(), &character_game_data::traits_changed, this, &character_data_model::update_trait_rows);
		connect(this->character->get_game_data(), &character_game_data::wealth_changed, this, &character_data_model::update_wealth_row);
	}

	emit character_changed();
}

void character_data_model::reset_model()
{
	this->beginResetModel();
	this->resetting_model = true;

	this->top_rows.clear();
	this->attribute_row = nullptr;
	this->mana_row = nullptr;
	this->craft_row = nullptr;
	this->armor_class_row = nullptr;
	this->to_hit_bonus_row = nullptr;
	this->damage_row = nullptr;
	this->range_row = nullptr;
	this->movement_row = nullptr;
	this->initiative_bonus_row = nullptr;
	this->saving_throw_row = nullptr;
	this->skill_row = nullptr;
	this->trait_row = nullptr;
	this->wealth_row = nullptr;

	if (this->character != nullptr) {
		const character_game_data *character_game_data = this->get_character()->get_game_data();

		if (character_game_data->is_deity()) {
			const deity *deity = this->character->get_deity();

			this->top_rows.push_back(std::make_unique<character_data_row>("Divine Rank:", std::format("{} ({})", deity->get_divine_rank_name(), deity->get_divine_level())));

			this->top_rows.push_back(std::make_unique<character_data_row>("Pantheon:", deity->get_pantheon()->get_name()));

			this->create_divine_domain_rows();
		}

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

		const mythic_path *mythic_path = this->character->get_mythic_path();
		if (mythic_path != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Mythic Path:", mythic_path->get_name()));

			const int mythic_tier = this->character->get_mythic_tier();
			this->top_rows.push_back(std::make_unique<character_data_row>("Mythic Tier:", std::to_string(mythic_tier)));
		}

		if (this->character->get_dynasty() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Dynasty:", this->character->get_dynasty()->get_name()));
		}

		if (character_game_data->get_bloodline() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Bloodline:", std::format("{} ({})", character_game_data->get_bloodline()->get_cultural_name(this->character->get_culture()), character_game_data->get_bloodline_strength())));
		}

		std::string age_complement_str;
		if (character_game_data->is_dead()) {
			age_complement_str = std::format("Lived {}", game::get()->year_range_to_labeled_string(character_game_data->get_birth_date().year(), character_game_data->get_death_date().year()));
		} else {
			age_complement_str = std::format("Born in {}", game::get()->year_to_labeled_string(character_game_data->get_birth_date().year()));
		}
		this->top_rows.push_back(std::make_unique<character_data_row>("Age:", std::format("{} ({})", number::to_formatted_string(character_game_data->get_age()), age_complement_str)));

		if (this->character->get_culture() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Culture:", this->character->get_culture()->get_name()));
		}

		if (!character_game_data->is_deity() && this->character->get_religion() != nullptr) {
			this->top_rows.push_back(std::make_unique<character_data_row>("Religion:", this->character->get_religion()->get_name()));
		}

		this->top_rows.push_back(std::make_unique<character_data_row>("Reputation:", std::to_string(character_game_data->get_reputation())));

		this->top_rows.push_back(std::make_unique<character_data_row>("Challenge Rating:", std::to_string(character_game_data->get_challenge_rating())));

		this->create_attribute_rows();

		this->top_rows.push_back(std::make_unique<character_data_row>("Health:", std::format("{}/{}", character_game_data->get_health(), character_game_data->get_max_health())));

		if (character_game_data->get_max_mana() > 0) {
			this->create_mana_row();
		}

		if (character_game_data->get_max_craft() > 0 && character_game_data->can_craft_items()) {
			this->create_craft_row();
		}

		this->create_armor_class_rows();
		this->create_to_hit_bonus_rows();
		this->create_damage_row();
		this->create_range_row();
		this->create_movement_row();
		this->create_initiative_bonus_row();
		this->create_saving_throw_rows();
		this->create_skill_rows();
		this->create_trait_rows();

		if (character_game_data->exists()) {
			this->create_wealth_row();
		}
	}

	this->resetting_model = false;
	this->endResetModel();
}

void character_data_model::create_divine_domain_rows()
{
	if (this->get_character()->get_deity()->get_major_domains().empty() && this->get_character()->get_deity()->get_minor_domains().empty()) {
		return;
	}

	auto top_row = std::make_unique<character_data_row>("Divine Domains");

	for (const divine_domain *domain : this->get_character()->get_deity()->get_major_domains()) {
		auto row = std::make_unique<character_data_row>(domain->get_name(), "", top_row.get());
		top_row->child_rows.push_back(std::move(row));
	}

	for (const divine_domain *domain : this->get_character()->get_deity()->get_minor_domains()) {
		auto row = std::make_unique<character_data_row>(domain->get_name() + " (Minor)", "", top_row.get());
		top_row->child_rows.push_back(std::move(row));
	}

	this->top_rows.push_back(std::move(top_row));
}

void character_data_model::create_attribute_rows()
{
	auto row = std::make_unique<character_data_row>("Attributes");
	this->attribute_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_attribute_rows();
}

void character_data_model::update_attribute_rows()
{
	assert_throw(this->attribute_row != nullptr);

	this->clear_child_rows(this->attribute_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	for (const auto &[attribute, value] : character_game_data->get_attribute_values()) {
		auto row = std::make_unique<character_data_row>(attribute->get_name() + ":", std::to_string(value), this->attribute_row);
		this->attribute_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->attribute_row);
}

void character_data_model::create_mana_row()
{
	auto row = std::make_unique<character_data_row>("Mana:");
	this->mana_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_mana_row();
}

void character_data_model::update_mana_row()
{
	if (this->mana_row == nullptr) {
		return;
	}

	assert_throw(this->mana_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	if (character_game_data->exists()) {
		this->mana_row->value = std::format("{}/{}", character_game_data->get_mana(), character_game_data->get_max_mana());
	} else {
		this->mana_row->value = std::to_string(character_game_data->get_max_mana());
	}
}

void character_data_model::create_craft_row()
{
	auto row = std::make_unique<character_data_row>("Craft:");
	this->craft_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_craft_row();
}

void character_data_model::update_craft_row()
{
	if (this->craft_row == nullptr) {
		return;
	}

	assert_throw(this->craft_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	if (character_game_data->exists()) {
		this->craft_row->value = std::format("{}/{}", character_game_data->get_craft(), character_game_data->get_max_craft());
	} else {
		this->craft_row->value = std::to_string(character_game_data->get_max_craft());
	}
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
	auto row = std::make_unique<character_data_row>("To Hit:");
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

	dice damage_dice = character_game_data->get_damage_dice();
	damage_dice.change_modifier(character_game_data->get_damage_bonus());
	this->damage_row->value = damage_dice.to_display_string();
}

void character_data_model::create_range_row()
{
	auto row = std::make_unique<character_data_row>("Range:");
	this->range_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_range_row();
}

void character_data_model::update_range_row()
{
	assert_throw(this->range_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->range_row->value = std::to_string(character_game_data->get_range());
}

void character_data_model::create_movement_row()
{
	auto row = std::make_unique<character_data_row>("Movement:");
	this->movement_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_movement_row();
}

void character_data_model::update_movement_row()
{
	assert_throw(this->movement_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->movement_row->value = std::to_string(character_game_data->get_combat_movement());
}

void character_data_model::create_initiative_bonus_row()
{
	auto row = std::make_unique<character_data_row>("Initiative:");
	this->initiative_bonus_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_initiative_bonus_row();
}

void character_data_model::update_initiative_bonus_row()
{
	assert_throw(this->initiative_bonus_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->initiative_bonus_row->value = number::to_signed_string(character_game_data->get_initiative_bonus());
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

	data_entry_map<saving_throw_type, character_data_row *> saving_throw_type_rows;

	for (const auto &[saving_throw_type, bonus] : character_game_data->get_saving_throw_bonuses()) {
		this->create_saving_throw_row(saving_throw_type, bonus, saving_throw_type_rows);
	}

	this->on_child_rows_inserted(this->saving_throw_row);
}

void character_data_model::create_saving_throw_row(const saving_throw_type *saving_throw_type, const int bonus, data_entry_map<metternich::saving_throw_type, character_data_row *> &saving_throw_type_rows)
{
	character_data_row *parent_row = nullptr;

	if (saving_throw_type->get_base_saving_throw_type() != nullptr) {
		if (!saving_throw_type_rows.contains(saving_throw_type->get_base_saving_throw_type())) {
			this->create_saving_throw_row(saving_throw_type->get_base_saving_throw_type(), this->character->get_game_data()->get_saving_throw_bonus(saving_throw_type->get_base_saving_throw_type()), saving_throw_type_rows);
		}

		parent_row = saving_throw_type_rows.find(saving_throw_type->get_base_saving_throw_type())->second;
	} else {
		parent_row = this->saving_throw_row;
	}

	auto row = std::make_unique<character_data_row>(saving_throw_type->get_name() + ":", number::to_signed_string(bonus), parent_row);
	saving_throw_type_rows[saving_throw_type] = row.get();
	parent_row->child_rows.push_back(std::move(row));
}

void character_data_model::create_skill_rows()
{
	auto row = std::make_unique<character_data_row>("Skills");
	this->skill_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_skill_rows();
}

void character_data_model::update_skill_rows()
{
	assert_throw(this->skill_row != nullptr);

	this->clear_child_rows(this->skill_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	for (const auto &[skill, value] : character_game_data->get_skill_values()) {
		if (!character_game_data->is_skill_trained(skill)) {
			continue;
		}

		auto row = std::make_unique<character_data_row>(skill->get_name() + ":", std::format("{}{}", value, skill->get_value_suffix()), this->skill_row);
		this->skill_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->skill_row);
}

void character_data_model::create_trait_rows()
{
	auto row = std::make_unique<character_data_row>("Traits");
	this->trait_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_trait_rows();
}

void character_data_model::update_trait_rows()
{
	assert_throw(this->trait_row != nullptr);

	this->clear_child_rows(this->trait_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	data_entry_map<trait_type, std::vector<const trait *>> traits_by_type;

	for (const auto &[trait, count] : character_game_data->get_trait_counts()) {
		traits_by_type[trait->get_types().at(0)].push_back(trait);
	}

	for (const auto &[trait_type, traits] : traits_by_type) {
		auto trait_type_row = std::make_unique<character_data_row>(string::get_plural_form(trait_type->get_name()), "", this->trait_row);

		for (const trait *trait : traits) {
			const int trait_count = character_game_data->get_trait_count(trait);
			auto row = std::make_unique<character_data_row>(trait->get_name(), (trait->is_unlimited() && trait_count > 1 ? std::format("(x{})", trait_count) : ""), trait_type_row.get());
			row->tooltip = trait->get_modifier_string(trait_count, true);
			trait_type_row->child_rows.push_back(std::move(row));
		}

		this->trait_row->child_rows.push_back(std::move(trait_type_row));
	}

	this->on_child_rows_inserted(this->trait_row);
}

void character_data_model::create_wealth_row()
{
	auto row = std::make_unique<character_data_row>("Wealth:");
	this->wealth_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_wealth_row();
}

void character_data_model::update_wealth_row()
{
	assert_throw(this->wealth_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->wealth_row->value = defines::get()->get_wealth_commodity()->value_to_string(character_game_data->get_wealth());
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
