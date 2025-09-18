#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("game/game_rule.h")
Q_MOC_INCLUDE("technology/technology.h")

namespace archimedes {
	class dice;
	class game_rule;
}

namespace metternich {

class commodity_unit;
class icon;
class technology;
enum class commodity_type;
enum class food_type;

class commodity final : public named_data_entry, public data_type<commodity>
{
	Q_OBJECT

	Q_PROPERTY(metternich::commodity_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::food_type food_type MEMBER food_type READ get_food_type NOTIFY changed)
	Q_PROPERTY(bool abstract MEMBER abstract READ is_abstract NOTIFY changed)
	Q_PROPERTY(bool storable MEMBER storable READ is_storable NOTIFY changed)
	Q_PROPERTY(bool local MEMBER local READ is_local NOTIFY changed)
	Q_PROPERTY(bool provincial MEMBER provincial READ is_provincial NOTIFY changed)
	Q_PROPERTY(bool negative_allowed MEMBER negative_allowed READ is_negative_allowed NOTIFY changed)
	Q_PROPERTY(bool housing MEMBER housing READ is_housing NOTIFY changed)
	Q_PROPERTY(int wealth_value MEMBER wealth_value READ get_wealth_value NOTIFY changed)
	Q_PROPERTY(int base_price MEMBER base_price READ get_base_price NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(const archimedes::game_rule* required_game_rule MEMBER required_game_rule NOTIFY changed)
	Q_PROPERTY(bool enabled READ is_enabled NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "commodity";
	static constexpr const char property_class_identifier[] = "metternich::commodity*";
	static constexpr const char database_folder[] = "commodities";

	static constexpr int abstract_commodity_value = 100; //used for e.g. score calculation

	explicit commodity(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	commodity_type get_type() const
	{
		return this->type;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_food() const;

	metternich::food_type get_food_type() const
	{
		return this->food_type;
	}

	bool is_abstract() const
	{
		return this->abstract;
	}

	bool is_storable() const
	{
		return this->storable;
	}

	bool is_local() const
	{
		return this->local;
	}

	bool is_provincial() const
	{
		return this->provincial;
	}

	bool is_negative_allowed() const
	{
		return this->negative_allowed;
	}

	bool is_housing() const
	{
		return this->housing;
	}

	int get_wealth_value() const
	{
		return this->wealth_value;
	}

	bool is_convertible_to_wealth() const
	{
		return this->get_wealth_value() != 0;
	}

	int get_base_price() const
	{
		return this->base_price;
	}

	bool is_tradeable() const
	{
		return !this->is_abstract() && this->is_storable() && !this->is_convertible_to_wealth();
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	bool is_enabled() const;

	bool has_unit(const commodity_unit *unit) const
	{
		return this->unit_values.contains(unit);
	}

	Q_INVOKABLE const metternich::commodity_unit *get_unit(const int value) const;
	Q_INVOKABLE int get_unit_value(const metternich::commodity_unit *unit) const;

	std::pair<std::string, const commodity_unit *> string_to_number_string_and_unit(const std::string &str) const;
	int string_to_value(const std::string &str) const;
	std::pair<std::variant<int, dice>, const commodity_unit *> string_to_value_variant_with_unit(const std::string &str) const;

	std::string value_to_string(const int value) const;
	Q_INVOKABLE QString value_to_qstring(const int value) const;

	Q_INVOKABLE QString get_units_tooltip() const;

signals:
	void changed();

private:
	commodity_type type {};
	metternich::icon *icon = nullptr;
	metternich::food_type food_type;
	bool abstract = false;
	bool storable = true;
	bool local = false;
	bool provincial = false;
	bool negative_allowed = false;
	bool housing = false;
	int wealth_value = 0;
	int base_price = 0;
	technology *required_technology = nullptr;
	const game_rule *required_game_rule = nullptr;
	std::map<int, const commodity_unit *> units;
	data_entry_map<commodity_unit, int> unit_values;
};

}
