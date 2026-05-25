#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/centesimal_int.h"
#include "util/decimillesimal_int.h"

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
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::icon* tiny_icon MEMBER tiny_icon READ get_tiny_icon NOTIFY changed)
	Q_PROPERTY(metternich::food_type food_type MEMBER food_type READ get_food_type NOTIFY changed)
	Q_PROPERTY(bool abstract MEMBER abstract READ is_abstract NOTIFY changed)
	Q_PROPERTY(bool manpower MEMBER manpower READ is_manpower NOTIFY changed)
	Q_PROPERTY(bool agricultural MEMBER agricultural READ is_agricultural NOTIFY changed)
	Q_PROPERTY(bool mineral MEMBER mineral READ is_mineral NOTIFY changed)
	Q_PROPERTY(bool industrial MEMBER industrial READ is_industrial NOTIFY changed)
	Q_PROPERTY(bool storable MEMBER storable READ is_storable NOTIFY changed)
	Q_PROPERTY(bool special_storage_capacity MEMBER special_storage_capacity READ has_special_storage_capacity NOTIFY changed)
	Q_PROPERTY(bool local MEMBER local READ is_local NOTIFY changed)
	Q_PROPERTY(bool provincial MEMBER provincial READ is_provincial NOTIFY changed)
	Q_PROPERTY(bool negative_allowed MEMBER negative_allowed READ is_negative_allowed NOTIFY changed)
	Q_PROPERTY(qint64 wealth_value MEMBER wealth_value READ get_wealth_value NOTIFY changed)
	Q_PROPERTY(qint64 base_price MEMBER base_price READ get_base_price NOTIFY changed)
	Q_PROPERTY(const metternich::commodity_unit* storage_unit MEMBER storage_unit READ get_storage_unit NOTIFY changed)
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

	const metternich::icon *get_tiny_icon() const
	{
		return this->tiny_icon;
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

	bool is_manpower() const
	{
		return this->manpower;
	}

	bool is_agricultural() const
	{
		return this->agricultural;
	}

	bool is_mineral() const
	{
		return this->mineral;
	}

	bool is_industrial() const
	{
		return this->industrial;
	}

	bool is_storable() const
	{
		return this->storable;
	}

	bool has_special_storage_capacity() const
	{
		return this->special_storage_capacity;
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

	int64_t get_wealth_value() const
	{
		return this->wealth_value;
	}

	bool is_convertible_to_wealth() const
	{
		return this->get_wealth_value() != 0;
	}

	int64_t get_base_price() const
	{
		return this->base_price;
	}

	bool is_tradeable() const
	{
		return !this->is_abstract() && this->is_storable() && !this->is_convertible_to_wealth();
	}

	Q_INVOKABLE bool is_wealth() const;

	const commodity_unit *get_storage_unit() const
	{
		return this->storage_unit;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	bool is_enabled() const;

	const std::map<int64_t, const commodity_unit *> &get_units() const
	{
		return this->units;
	}

	bool has_unit(const commodity_unit *unit) const
	{
		return this->unit_values.contains(unit);
	}

	Q_INVOKABLE const metternich::commodity_unit *get_unit(const qint64 value) const;
	Q_INVOKABLE qint64 get_unit_value(const metternich::commodity_unit *unit) const;

	std::pair<std::string, const commodity_unit *> string_to_number_string_and_unit(const std::string &str) const;
	decimillesimal_int string_to_fractional_value(const std::string &str) const;
	int64_t string_to_value(const std::string &str) const;
	std::pair<std::variant<int64_t, dice>, const commodity_unit *> string_to_value_variant_with_unit(const std::string &str) const;

	std::string value_to_string(const int64_t value, const bool joined) const;
	std::string value_to_string(const centesimal_int &value, const bool joined) const;
	Q_INVOKABLE QString value_to_qstring(const qint64 value) const;

	Q_INVOKABLE QString get_units_tooltip() const;

	int64_t wealth_value_to_value(const int64_t wealth_value) const;

signals:
	void changed();

private:
	commodity_type type {};
	const metternich::icon *icon = nullptr;
	const metternich::icon *tiny_icon = nullptr;
	metternich::food_type food_type;
	bool abstract = false;
	bool manpower = false; //whether this is some form of manpower, used for recruiting units
	bool agricultural = false; //benefits from farming efficiency gains
	bool mineral = false; //benefits from mining efficiency gains
	bool industrial = false; //benefits from industrial efficiency gains
	bool storable = true;
	bool special_storage_capacity = false; //whether this commodity has special storage capacity for itself
	bool local = false;
	bool provincial = false;
	bool negative_allowed = false;
	int64_t wealth_value = 0;
	int64_t base_price = 0;
	const commodity_unit *storage_unit = nullptr;
	technology *required_technology = nullptr;
	const game_rule *required_game_rule = nullptr;
	std::map<int64_t, const commodity_unit *> units;
	data_entry_map<commodity_unit, int64_t> unit_values;
};

}
