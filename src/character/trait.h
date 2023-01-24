#pragma once

#include "character/character_type_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class character_type;
class icon;
enum class attribute;
enum class trait_type;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class trait final : public named_data_entry, public data_type<trait>
{
	Q_OBJECT

	Q_PROPERTY(metternich::trait_type type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "trait";
	static constexpr const char property_class_identifier[] = "metternich::trait*";
	static constexpr const char database_folder[] = "traits";

	explicit trait(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	trait_type get_type() const
	{
		return this->type;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_level() const
	{
		return this->level;
	}

	const condition<character> *get_conditions() const
	{
		return this->conditions.get();
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	const metternich::modifier<const character> *get_character_type_modifier(const character_type *character_type) const
	{
		const auto find_iterator = this->character_type_modifiers.find(character_type);
		if (find_iterator != this->character_type_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	QString get_modifier_string() const;
	Q_INVOKABLE QString get_modifier_string(metternich::character_type *character_type) const;

signals:
	void changed();

private:
	trait_type type;
	metternich::icon *icon = nullptr;
	int level = 0;
	std::unique_ptr<const condition<character>> conditions;
	std::unique_ptr<metternich::modifier<const character>> modifier;
	character_type_map<std::unique_ptr<metternich::modifier<const character>>> character_type_modifiers;
};

}
