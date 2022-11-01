#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class commodity;
class resource;
class terrain_type;

//tile improvement
class improvement final : public named_data_entry, public data_type<improvement>
{
	Q_OBJECT

	Q_PROPERTY(metternich::resource* resource MEMBER resource)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(int output_value MEMBER output_value READ get_output_value)

public:
	static constexpr const char class_identifier[] = "improvement";
	static constexpr const char property_class_identifier[] = "metternich::improvement*";
	static constexpr const char database_folder[] = "improvements";

	explicit improvement(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::resource *get_resource() const
	{
		return this->resource;
	}

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);

	const commodity *get_output_commodity() const;

	int get_output_value() const
	{
		return this->output_value;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

private:
	metternich::resource *resource = nullptr; //the resource for which this improvement can be built
	std::filesystem::path image_filepath;
	int output_value = 0;
	std::vector<const terrain_type *> terrain_types; //the terrain types where the improvement can be built
};

}
