#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class tradition;

class tradition_group final : public named_data_entry, public data_type<tradition_group>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "tradition_group";
	static constexpr const char property_class_identifier[] = "metternich::tradition_group*";
	static constexpr const char database_folder[] = "tradition_groups";

	explicit tradition_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::vector<const tradition *> &get_traditions() const
	{
		return this->traditions;
	}

	void add_tradition(const tradition *tradition)
	{
		this->traditions.push_back(tradition);
	}

signals:
	void changed();

private:
	std::vector<const tradition *> traditions;
};

}
