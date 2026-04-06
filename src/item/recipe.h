#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class enchantment;
class item;
class item_type;
class spell;
class trait;
struct item_key;

template <typename scope_type>
class and_condition;

struct recipe_material final
{
	explicit recipe_material(const metternich::item_type *item_type, const metternich::enchantment *enchantment, const int quantity = 1)
		: item_type(item_type), enchantment(enchantment), quantity(quantity)
	{
	}

	bool matches_item(const item_key &item_key) const;

	const metternich::item_type *item_type = nullptr;
	const metternich::enchantment *enchantment = nullptr;
	int quantity = 1;
};

class recipe final : public named_data_entry, public data_type<recipe>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::item_type* result_item_type MEMBER result_item_type READ get_result_item_type NOTIFY changed)
	Q_PROPERTY(const metternich::enchantment* result_enchantment MEMBER result_enchantment READ get_result_enchantment NOTIFY changed)
	Q_PROPERTY(int min_caster_level MEMBER min_caster_level READ get_min_caster_level NOTIFY changed)
	Q_PROPERTY(QString formula_string READ get_formula_qstring NOTIFY changed)
	

public:
	static constexpr const char class_identifier[] = "recipe";
	static constexpr const char property_class_identifier[] = "metternich::recipe*";
	static constexpr const char database_folder[] = "recipes";

	explicit recipe(const std::string &identifier);
	~recipe();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const icon *get_icon() const;

	const item_type *get_result_item_type() const
	{
		return this->result_item_type;
	}

	const enchantment *get_result_enchantment() const
	{
		return this->result_enchantment;
	}

	item_key get_result_item_key() const;

	int get_min_caster_level() const
	{
		return this->min_caster_level;
	}

	const std::vector<const trait *> &get_required_traits() const
	{
		return this->required_traits;
	}

	const and_condition<character> *get_crafter_conditions() const
	{
		return this->crafter_conditions.get();
	}

	const std::vector<recipe_material> &get_materials() const
	{
		return this->materials;
	}

	void add_material(const item_type *item_type, const enchantment *enchantment);

	const std::vector<const spell *> &get_spells() const
	{
		return this->spells;
	}

	int get_price() const;

	int get_price_of_materials() const;
	int get_result_price() const;

	int get_craft_cost() const;

	std::string get_formula_string() const;

	Q_INVOKABLE QString get_formula_qstring() const
	{
		return QString::fromStdString(this->get_formula_string());
	}

signals:
	void changed();

private:
	const item_type *result_item_type = nullptr;
	const enchantment *result_enchantment = nullptr;
	int min_caster_level = 0;
	std::vector<const trait *> required_traits;
	std::unique_ptr<const and_condition<character>> crafter_conditions;
	std::vector<recipe_material> materials;
	std::vector<const spell *> spells;
};

}
