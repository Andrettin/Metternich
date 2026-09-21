#include "metternich.h"

#include "character/character_data_model.h"

#include "character/bloodline.h"
#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_attribute_type.h"
#include "character/character_class.h"
#include "character/character_defines.h"
#include "character/character_game_data.h"
#include "character/damage_reduction_type.h"
#include "character/domain_skill.h"
#include "character/dynasty.h"
#include "character/mythic_path.h"
#include "character/save_type.h"
#include "character/skill.h"
#include "character/trait.h"
#include "character/trait_type.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "item/item.h"
#include "item/item_type.h"
#include "religion/deity.h"
#include "religion/divine_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "species/creature_size.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/number_util.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

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
		disconnect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_attribute_rows);
		disconnect(this->character->get_game_data(), &character_game_data::exceptional_attribute_values_changed, this, &character_data_model::update_attribute_rows);
		disconnect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_personality_rows);
		disconnect(this->character->get_game_data(), &character_game_data::mana_changed, this, &character_data_model::update_mana_row);
		disconnect(this->character->get_game_data(), &character_game_data::max_mana_changed, this, &character_data_model::update_mana_row);
		disconnect(this->character->get_game_data(), &character_game_data::craft_changed, this, &character_data_model::update_craft_row);
		disconnect(this->character->get_game_data(), &character_game_data::max_craft_changed, this, &character_data_model::update_craft_row);
		disconnect(this->character->get_game_data(), &character_game_data::weight_changed, this, &character_data_model::update_size_row);
		disconnect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		disconnect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		disconnect(this->character->get_game_data(), &character_game_data::weapon_to_hit_bonuses_changed, this, &character_data_model::update_to_hit_bonus_rows);
		disconnect(this->character->get_game_data(), &character_game_data::damage_bonus_changed, this, &character_data_model::update_damage_rows);
		disconnect(this->character->get_game_data(), &character_game_data::weapon_damage_bonuses_changed, this, &character_data_model::update_damage_rows);
		disconnect(this->character->get_game_data(), &character_game_data::range_changed, this, &character_data_model::update_range_row);
		disconnect(this->character->get_game_data(), &character_game_data::movement_changed, this, &character_data_model::update_movement_row);
		disconnect(this->character->get_game_data(), &character_game_data::initiative_bonus_changed, this, &character_data_model::update_initiative_bonus_row);
		disconnect(this->character->get_game_data(), &character_game_data::damage_reductions_changed, this, &character_data_model::update_damage_reduction_rows);
		disconnect(this->character->get_game_data(), &character_game_data::save_bonuses_changed, this, &character_data_model::update_save_rows);
		disconnect(this->character->get_game_data(), &character_game_data::skill_trainings_changed, this, &character_data_model::update_skill_rows);
		disconnect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_skill_rows);
		disconnect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_domain_skill_rows);
		disconnect(this->character->get_game_data(), &character_game_data::traits_changed, this, &character_data_model::update_trait_rows);
		disconnect(this->character->get_game_data(), &character_game_data::wealth_changed, this, &character_data_model::update_wealth_row);
		disconnect(this->character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::update_damage_rows);
	}

	this->character = character;

	this->reset_model();

	if (character != nullptr) {
		connect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_attribute_rows);
		connect(this->character->get_game_data(), &character_game_data::exceptional_attribute_values_changed, this, &character_data_model::update_attribute_rows);
		connect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_personality_rows);
		connect(this->character->get_game_data(), &character_game_data::mana_changed, this, &character_data_model::update_mana_row);
		connect(this->character->get_game_data(), &character_game_data::max_mana_changed, this, &character_data_model::update_mana_row);
		connect(this->character->get_game_data(), &character_game_data::craft_changed, this, &character_data_model::update_craft_row);
		connect(this->character->get_game_data(), &character_game_data::max_craft_changed, this, &character_data_model::update_craft_row);
		connect(this->character->get_game_data(), &character_game_data::weight_changed, this, &character_data_model::update_size_row);
		connect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::species_armor_class_bonuses_changed, this, &character_data_model::update_armor_class_rows);
		connect(this->character->get_game_data(), &character_game_data::to_hit_bonus_changed, this, &character_data_model::update_to_hit_bonus_rows);
		connect(this->character->get_game_data(), &character_game_data::weapon_to_hit_bonuses_changed, this, &character_data_model::update_to_hit_bonus_rows);
		connect(this->character->get_game_data(), &character_game_data::damage_bonus_changed, this, &character_data_model::update_damage_rows);
		connect(this->character->get_game_data(), &character_game_data::weapon_damage_bonuses_changed, this, &character_data_model::update_damage_rows);
		connect(this->character->get_game_data(), &character_game_data::range_changed, this, &character_data_model::update_range_row);
		connect(this->character->get_game_data(), &character_game_data::movement_changed, this, &character_data_model::update_movement_row);
		connect(this->character->get_game_data(), &character_game_data::initiative_bonus_changed, this, &character_data_model::update_initiative_bonus_row);
		connect(this->character->get_game_data(), &character_game_data::damage_reductions_changed, this, &character_data_model::update_damage_reduction_rows);
		connect(this->character->get_game_data(), &character_game_data::save_bonuses_changed, this, &character_data_model::update_save_rows);
		connect(this->character->get_game_data(), &character_game_data::skill_trainings_changed, this, &character_data_model::update_skill_rows);
		connect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_skill_rows);
		connect(this->character->get_game_data(), &character_game_data::stat_values_changed, this, &character_data_model::update_domain_skill_rows);
		connect(this->character->get_game_data(), &character_game_data::traits_changed, this, &character_data_model::update_trait_rows);
		connect(this->character->get_game_data(), &character_game_data::wealth_changed, this, &character_data_model::update_wealth_row);
		connect(this->character->get_game_data(), &character_game_data::equipped_items_changed, this, &character_data_model::update_damage_rows);
	}

	emit character_changed();
}

void character_data_model::reset_model()
{
	this->beginResetModel();
	this->resetting_model = true;

	this->top_rows.clear();
	this->size_row = nullptr;
	this->attribute_type_rows.clear();
	this->mana_row = nullptr;
	this->craft_row = nullptr;
	this->armor_class_row = nullptr;
	this->to_hit_bonus_row = nullptr;
	this->damage_row = nullptr;
	this->range_row = nullptr;
	this->movement_row = nullptr;
	this->initiative_bonus_row = nullptr;
	this->damage_reduction_row = nullptr;
	this->save_row = nullptr;
	this->skill_row = nullptr;
	this->domain_skill_row = nullptr;
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

			if (character_game_data->get_level() < character_game_data->get_max_level()) {
				this->top_rows.push_back(std::make_unique<character_data_row>("Experience:", std::format("{}/{}", number::to_formatted_string(character_game_data->get_experience()), number::to_formatted_string(character_game_data->get_experience_for_next_level()))));
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

		if (!character_game_data->is_deity()) {
			if (this->character->get_religion() != nullptr) {
				this->top_rows.push_back(std::make_unique<character_data_row>("Religion:", this->character->get_religion()->get_name()));
			}

			if (character_game_data->get_patron_deity() != nullptr) {
				auto row = std::make_unique<character_data_row>(std::format("Patron {}:", this->character->get_religion()->is_monotheistic() ? "Saint" : "Deity"), character_game_data->get_patron_deity()->get_name());

				std::string tooltip = "Divine Domains: ";
				bool first = true;
				for (const divine_domain *domain : character_game_data->get_patron_deity()->get_major_domains()) {
					if (first) {
						first = false;
					} else {
						tooltip += ", ";
					}

					tooltip += domain->get_name();
				}
				for (const divine_domain *domain : character_game_data->get_patron_deity()->get_minor_domains()) {
					if (first) {
						first = false;
					} else {
						tooltip += ", ";
					}

					tooltip += domain->get_name() + " (Minor)";
				}
				if (first) {
					tooltip += "None";
				}

				row->tooltip = std::move(tooltip);
				this->top_rows.push_back(std::move(row));
			}
		}

		this->top_rows.push_back(std::make_unique<character_data_row>("Reputation:", std::to_string(character_game_data->get_reputation())));

		this->top_rows.push_back(std::make_unique<character_data_row>("Challenge Rating:", std::to_string(character_game_data->get_challenge_rating())));

		this->create_size_row();

		this->create_attribute_type_rows(character_attribute_type::main);
		this->create_attribute_type_rows(character_attribute_type::personality);

		this->top_rows.push_back(std::make_unique<character_data_row>("Health:", std::format("{}/{}", character_game_data->get_health(), character_game_data->get_max_health())));

		if (character_game_data->get_max_mana() > 0) {
			this->create_mana_row();
		}

		if (character_game_data->get_max_craft() > 0 && character_game_data->can_craft_items()) {
			this->create_craft_row();
		}

		this->create_armor_class_rows();
		this->create_to_hit_bonus_rows();
		this->create_damage_rows();
		this->create_range_row();
		this->create_movement_row();
		this->create_initiative_bonus_row();
		if (!character_game_data->get_damage_reductions().empty()) {
			this->create_damage_reduction_rows();
		}
		this->create_save_rows();
		this->create_skill_rows();
		if (character_game_data->has_domain_skill()) {
			this->create_domain_skill_rows();
		}
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

void character_data_model::create_size_row()
{
	auto row = std::make_unique<character_data_row>("Size:");
	this->size_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_size_row();
}

void character_data_model::update_size_row()
{
	assert_throw(this->size_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	this->size_row->value = std::format("{} ({})", character_game_data->get_creature_size()->get_name(), string::from_weight(character_game_data->get_weight(), false, preferences::get()->are_metric_measurements_enabled()));

	this->on_top_row_changed(this->size_row);
}

void character_data_model::create_attribute_type_rows(const character_attribute_type type)
{
	std::string_view attribute_row_name;

	switch (type) {
		case character_attribute_type::main:
			attribute_row_name = "Attributes";
			break;
		case character_attribute_type::personality:
			attribute_row_name = "Personality";
			break;
		default:
			assert_throw(false);
			break;
	}

	auto row = std::make_unique<character_data_row>(std::string(attribute_row_name));
	this->attribute_type_rows[type] = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_attribute_type_rows(type);
}

void character_data_model::update_attribute_type_rows(const character_attribute_type type)
{
	character_data_row *attribute_type_row = this->attribute_type_rows[type];

	assert_throw(attribute_type_row != nullptr);

	this->clear_child_rows(attribute_type_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	data_entry_map<character_attribute, character_data_row *> attribute_rows;

	for (const auto &[stat, value] : character_game_data->get_stat_values()) {
		const character_attribute *attribute = dynamic_cast<const character_attribute *>(stat);
		if (attribute == nullptr) {
			continue;
		}

		if (attribute->get_type() != type) {
			continue;
		}

		this->create_attribute_row(attribute, value, attribute_rows);
	}

	//ensure attribute rows are sorted by name
	//since base attribute rows can be created by subattribute rows, they might be out of order otherwise
	std::sort(attribute_type_row->child_rows.begin(), attribute_type_row->child_rows.end(), [](const std::unique_ptr<character_data_row> &lhs, const std::unique_ptr<character_data_row> &rhs) {
		return lhs->name < rhs->name;
	});

	this->on_child_rows_inserted(attribute_type_row);
}

void character_data_model::create_attribute_row(const character_attribute *attribute, const int value, data_entry_map<character_attribute, character_data_row *> &attribute_rows)
{
	if (attribute_rows.contains(attribute)) {
		//already created by subattribute row
		return;
	}

	character_data_row *parent_row = nullptr;

	if (attribute->get_base_attribute() != nullptr && attribute->get_base_attribute()->get_type() == attribute->get_type()) {
		if (!attribute_rows.contains(attribute->get_base_attribute())) {
			this->create_attribute_row(attribute->get_base_attribute(), this->character->get_game_data()->get_attribute_value(attribute->get_base_attribute()), attribute_rows);
		}

		parent_row = attribute_rows.find(attribute->get_base_attribute())->second;
	} else {
		parent_row = this->attribute_type_rows[attribute->get_type()];
	}

	const int exceptional_value = this->character->get_game_data()->get_exceptional_attribute_value(attribute);
	auto row = std::make_unique<character_data_row>(attribute->get_name() + ":", attribute->is_value_exceptional(value) && exceptional_value > 0 ? std::format("{}/{}", value, exceptional_value % 100) : std::to_string(value), parent_row);
	attribute_rows[attribute] = row.get();
	parent_row->child_rows.push_back(std::move(row));
}

void character_data_model::update_attribute_rows()
{
	this->update_attribute_type_rows(character_attribute_type::main);
}

void character_data_model::update_personality_rows()
{
	this->update_attribute_type_rows(character_attribute_type::personality);
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
		auto row = std::make_unique<character_data_row>(std::format("Against {}:", string::get_plural_form(species->get_name())), number::to_signed_string(bonus), this->armor_class_row);
		this->armor_class_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->armor_class_row);

	this->on_top_row_changed(this->armor_class_row);
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

	this->clear_child_rows(this->to_hit_bonus_row);

	for (const auto &[weapon_type, bonus] : character_game_data->get_weapon_to_hit_bonuses()) {
		auto row = std::make_unique<character_data_row>(weapon_type->get_name(), number::to_signed_string(bonus), this->to_hit_bonus_row);
		this->to_hit_bonus_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->to_hit_bonus_row);

	this->on_top_row_changed(this->to_hit_bonus_row);
}

void character_data_model::create_damage_rows()
{
	auto row = std::make_unique<character_data_row>("Damage:");
	this->damage_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_damage_rows();
}

void character_data_model::update_damage_rows()
{
	assert_throw(this->damage_row != nullptr);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	const int min_damage = character_game_data->get_min_damage(character_defines::get()->get_default_creature_size());
	const int max_damage = character_game_data->get_max_damage(character_defines::get()->get_default_creature_size());

	this->damage_row->value = std::format("{}-{}", min_damage, max_damage);

	this->clear_child_rows(this->damage_row);

	creature_size_set weapon_target_creature_sizes;
	const std::vector<const item *> weapons = character_game_data->get_weapons();
	for (const item *weapon : weapons) {
		for (const auto &[creature_size, damage_dice] : weapon->get_type()->get_damage_dice_per_target_size()) {
			weapon_target_creature_sizes.insert(creature_size);
		}
	}

	for (const creature_size *creature_size : weapon_target_creature_sizes) {
		if (creature_size == character_defines::get()->get_default_creature_size()) {
			continue;
		}

		const int min_creature_size_damage = character_game_data->get_min_damage(creature_size);
		const int max_creature_size_damage = character_game_data->get_max_damage(creature_size);

		if (min_creature_size_damage == min_damage && max_creature_size_damage == max_damage) {
			continue;
		}

		auto row = std::make_unique<character_data_row>(std::format("Against {} Creatures:", creature_size->get_name()), std::format("{}-{}", min_creature_size_damage, max_creature_size_damage), this->damage_row);
		this->damage_row->child_rows.push_back(std::move(row));
	}

	for (const auto &[weapon_type, bonus] : character_game_data->get_weapon_damage_bonuses()) {
		auto row = std::make_unique<character_data_row>(weapon_type->get_name(), number::to_signed_string(bonus), this->damage_row);
		this->damage_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->damage_row);

	this->on_top_row_changed(this->damage_row);
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

	this->range_row->value = string::from_length(character_game_data->get_effective_range(), false, preferences::get()->are_metric_measurements_enabled());

	this->on_top_row_changed(this->range_row);
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

	this->movement_row->value = std::to_string(character_game_data->get_movement());

	this->on_top_row_changed(this->movement_row);
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

	this->on_top_row_changed(this->initiative_bonus_row);
}

void character_data_model::create_damage_reduction_rows()
{
	auto row = std::make_unique<character_data_row>("Damage Reduction");
	this->damage_reduction_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_damage_reduction_rows();
}

void character_data_model::update_damage_reduction_rows()
{
	assert_throw(this->damage_reduction_row != nullptr);

	this->damage_reduction_row->name = "Damage Reduction";

	this->clear_child_rows(this->damage_reduction_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	data_entry_map<damage_reduction_type, character_data_row *> damage_reduction_type_rows;

	for (const auto &[damage_reduction_type, value] : character_game_data->get_damage_reductions()) {
		if (damage_reduction_type->has_vulnerability()) {
			auto row = std::make_unique<character_data_row>(std::format("{}:", damage_reduction_type->get_name()), number::to_signed_string(value), this->damage_reduction_row);
			this->damage_reduction_row->child_rows.push_back(std::move(row));
		} else {
			this->damage_reduction_row->name = "Damage Reduction:";
			this->damage_reduction_row->value = number::to_signed_string(value);
		}
	}

	this->on_child_rows_inserted(this->damage_reduction_row);
}

void character_data_model::create_save_rows()
{
	auto row = std::make_unique<character_data_row>("Saves");
	this->save_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_save_rows();
}

void character_data_model::update_save_rows()
{
	assert_throw(this->save_row != nullptr);

	this->clear_child_rows(this->save_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	data_entry_map<save_type, character_data_row *> save_type_rows;

	for (const auto &[save_type, bonus] : character_game_data->get_save_bonuses()) {
		this->create_save_row(save_type, bonus, save_type_rows);
	}

	this->on_child_rows_inserted(this->save_row);
}

void character_data_model::create_save_row(const save_type *save_type, const int bonus, data_entry_map<metternich::save_type, character_data_row *> &save_type_rows)
{
	if (save_type_rows.contains(save_type)) {
		//already created by derived save row
		return;
	}

	character_data_row *parent_row = nullptr;

	if (save_type->get_base_save_type() != nullptr) {
		if (!save_type_rows.contains(save_type->get_base_save_type())) {
			this->create_save_row(save_type->get_base_save_type(), this->character->get_game_data()->get_save_bonus(save_type->get_base_save_type()), save_type_rows);
		}

		parent_row = save_type_rows.find(save_type->get_base_save_type())->second;
	} else {
		parent_row = this->save_row;
	}

	auto row = std::make_unique<character_data_row>(save_type->get_name() + ":", number::to_signed_string(bonus), parent_row);
	save_type_rows[save_type] = row.get();
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

	for (const auto &[stat, value] : character_game_data->get_stat_values()) {
		const skill *skill = dynamic_cast<const metternich::skill *>(stat);
		if (skill == nullptr) {
			continue;
		}

		if (!character_game_data->is_skill_available(skill)) {
			continue;
		}

		auto row = std::make_unique<character_data_row>(skill->get_name() + ":", std::format("{}{}", value, skill->get_value_suffix()), this->skill_row);
		this->skill_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->skill_row);
}

void character_data_model::create_domain_skill_rows()
{
	auto row = std::make_unique<character_data_row>("Domain Skills");
	this->domain_skill_row = row.get();
	this->top_rows.push_back(std::move(row));

	this->update_domain_skill_rows();
}

void character_data_model::update_domain_skill_rows()
{
	if (this->domain_skill_row == nullptr) {
		return;
	}

	this->clear_child_rows(this->domain_skill_row);

	const character_game_data *character_game_data = this->get_character()->get_game_data();

	for (const auto &[stat, value] : character_game_data->get_stat_values()) {
		const domain_skill *domain_skill = dynamic_cast<const metternich::domain_skill *>(stat);
		if (domain_skill == nullptr) {
			continue;
		}

		auto row = std::make_unique<character_data_row>(domain_skill->get_name() + ":", number::to_signed_string(value), this->domain_skill_row);
		this->domain_skill_row->child_rows.push_back(std::move(row));
	}

	this->on_child_rows_inserted(this->domain_skill_row);
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

	this->wealth_row->value = defines::get()->get_wealth_commodity()->value_to_string(character_game_data->get_wealth(), false);
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

void character_data_model::on_top_row_changed(character_data_row *row)
{
	if (this->resetting_model) {
		return;
	}

	const size_t row_index = this->get_top_row_index(row).value();
	const QModelIndex row_model_index = this->index(static_cast<int>(row_index), 0);
	emit dataChanged(row_model_index, row_model_index);
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
