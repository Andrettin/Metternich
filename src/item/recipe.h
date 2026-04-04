#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class enchantment;
class item_type;

template <typename scope_type>
class and_condition;

class recipe final : public named_data_entry, public data_type<recipe>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY changed)
	Q_PROPERTY(const metternich::item_type* result_item_type MEMBER result_item_type READ get_result_item_type NOTIFY changed)
	Q_PROPERTY(const metternich::enchantment* result_enchantment MEMBER result_enchantment READ get_result_enchantment NOTIFY changed)
	Q_PROPERTY(QString formula_string READ get_formula_qstring NOTIFY changed)
	

public:
	static constexpr const char class_identifier[] = "recipe";
	static constexpr const char property_class_identifier[] = "metternich::recipe*";
	static constexpr const char database_folder[] = "recipes";

	explicit recipe(const std::string &identifier);
	~recipe();

	virtual void process_gsml_scope(const gsml_data &scope) override;
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

	const and_condition<character> *get_crafter_conditions() const
	{
		return this->crafter_conditions.get();
	}

	int get_price() const;
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
	std::unique_ptr<const and_condition<character>> crafter_conditions;
};

}
