#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country_history;
class province;
enum class country_type;

class country final : public named_data_entry, public data_type<country>
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_type type MEMBER type READ get_type)
	Q_PROPERTY(QColor color MEMBER color READ get_color)
	Q_PROPERTY(metternich::province* capital_province MEMBER capital_province)

public:
	static constexpr const char class_identifier[] = "country";
	static constexpr const char property_class_identifier[] = "metternich::country*";
	static constexpr const char database_folder[] = "countries";
	static constexpr bool history_enabled = true;

	explicit country(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	country_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	country_type get_type() const
	{
		return this->type;
	}

	const QColor &get_color() const;

	const province *get_capital_province() const
	{
		return this->capital_province;
	}

	const std::vector<const province *> &get_provinces() const
	{
		return this->provinces;
	}

private:
	country_type type;
	QColor color;
	province *capital_province = nullptr;
	std::vector<const province *> provinces; //provinces for this country when it is generated in random maps
	std::unique_ptr<country_history> history;
};

}
