#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class law;

class law_group final : public named_data_entry, public data_type<law_group>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList laws READ get_laws_qvariant_list NOTIFY changed)
	Q_PROPERTY(const metternich::law* default_law MEMBER default_law READ get_default_law NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "law_group";
	static constexpr const char property_class_identifier[] = "metternich::law_group*";
	static constexpr const char database_folder[] = "law_groups";

	explicit law_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::vector<const law *> &get_laws() const
	{
		return this->laws;
	}

	QVariantList get_laws_qvariant_list() const;

	void add_law(const law *law)
	{
		this->laws.push_back(law);
	}

	const law *get_default_law() const
	{
		return this->default_law;
	}

signals:
	void changed();

private:
	std::vector<const law *> laws;
	const law *default_law = nullptr;
};

}
