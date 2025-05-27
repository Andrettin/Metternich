#pragma once

#include "database/data_type.h"
#include "species/taxon_base.h"

namespace metternich {

enum class taxonomic_rank;

class taxon final : public taxon_base, public data_type<taxon>
{
	Q_OBJECT

	Q_PROPERTY(QString common_name READ get_common_name_qstring)
	Q_PROPERTY(metternich::taxonomic_rank rank MEMBER rank READ get_rank)

public:
	static constexpr const char class_identifier[] = "taxon";
	static constexpr const char property_class_identifier[] = "metternich::taxon*";
	static constexpr const char database_folder[] = "taxons";

	explicit taxon(const std::string &identifier);

	virtual void check() const override;

	virtual const std::string &get_common_name() const override
	{
		if (!this->common_name.empty()) {
			return this->common_name;
		}

		return this->get_name();
	}

	QString get_common_name_qstring() const
	{
		return QString::fromStdString(this->common_name);
	}

	Q_INVOKABLE void set_common_name(const std::string &name)
	{
		this->common_name = name;
	}

	virtual taxonomic_rank get_rank() const override
	{
		return this->rank;
	}

private:
	std::string common_name;
	taxonomic_rank rank;
};

}
