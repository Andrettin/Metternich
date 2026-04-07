#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class military_unit;
enum class military_unit_stat;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class promotion final : public named_data_entry, public data_type<promotion>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "promotion";
	static constexpr const char property_class_identifier[] = "metternich::promotion*";
	static constexpr const char database_folder[] = "promotions";

	explicit promotion(const std::string &identifier);
	~promotion();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const and_condition<military_unit> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::map<military_unit_stat, int> &get_stat_bonuses() const
	{
		return this->stat_bonuses;
	}

	int get_stat_bonus(const military_unit_stat stat) const
	{
		const auto find_iterator = this->get_stat_bonuses().find(stat);
		if (find_iterator != this->get_stat_bonuses().end()) {
			return find_iterator->second;
		}

		return 0;
	}

	const metternich::modifier<military_unit> *get_modifier() const
	{
		return this->modifier.get();
	}

	[[nodiscard]] QCoro::Task<void> apply_modifier(military_unit *military_unit, const int multiplier) const;
	QString get_modifier_string() const;

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	std::unique_ptr<const and_condition<military_unit>> conditions;
	std::map<military_unit_stat, int> stat_bonuses;
	std::unique_ptr<const metternich::modifier<military_unit>> modifier;
};

}
