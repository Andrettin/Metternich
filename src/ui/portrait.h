#pragma once

#include "database/data_type.h"
#include "ui/icon_base.h"

namespace metternich {

class character;

template <typename scope_type>
class condition;

class portrait final : public icon_base, public data_type<portrait>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "portrait";
	static constexpr const char property_class_identifier[] = "metternich::portrait*";
	static constexpr const char database_folder[] = "portraits";

	static const std::vector<const portrait *> &get_character_portraits()
	{
		return portrait::character_portraits;
	}

private:
	static inline std::vector<const portrait *> character_portraits;

public:
	explicit portrait(const std::string &identifier);
	~portrait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const condition<character> *get_character_conditions() const
	{
		return this->character_conditions.get();
	}

	bool is_character_portrait() const
	{
		return this->get_character_conditions() != nullptr;
	}

private:
	std::unique_ptr<const condition<character>> character_conditions;
};

}
