#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/fractional_int.h"

namespace metternich {

class commodity;
class employment_type;
class resource;
class technology;
class terrain_type;

//tile improvement
class improvement final : public named_data_entry, public data_type<improvement>
{
	Q_OBJECT

	Q_PROPERTY(metternich::resource* resource MEMBER resource NOTIFY changed)
	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(metternich::employment_type* employment_type MEMBER employment_type NOTIFY changed)
	Q_PROPERTY(int employment_capacity MEMBER employment_capacity READ get_employment_capacity NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int output_multiplier MEMBER output_multiplier READ get_output_multiplier NOTIFY changed)
	Q_PROPERTY(int variation_count MEMBER variation_count READ get_variation_count)
	Q_PROPERTY(metternich::improvement* required_improvement MEMBER required_improvement NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "improvement";
	static constexpr const char property_class_identifier[] = "metternich::improvement*";
	static constexpr const char database_folder[] = "improvements";

	static constexpr int base_score = 10;

	explicit improvement(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
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

	const std::filesystem::path &get_terrain_image_filepath(const terrain_type *terrain) const
	{
		const auto find_iterator = this->terrain_image_filepaths.find(terrain);
		if (find_iterator != this->terrain_image_filepaths.end()) {
			return find_iterator->second;
		}

		return this->get_image_filepath();
	}

	bool has_terrain_image_filepath(const terrain_type *terrain) const
	{
		return this->terrain_image_filepaths.contains(terrain);
	}

	const metternich::employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	const commodity *get_output_commodity() const;

	centesimal_int get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

	int get_variation_count() const
	{
		return this->variation_count;
	}

	const improvement *get_required_improvement() const
	{
		return this->required_improvement;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_score() const
	{
		return (improvement::base_score * centesimal_int::max(centesimal_int(1), (this->get_employment_capacity() * this->get_output_multiplier()))).to_int();
	}

signals:
	void changed();

private:
	metternich::resource *resource = nullptr; //the resource for which this improvement can be built
	std::filesystem::path image_filepath;
	std::map<const terrain_type *, std::filesystem::path> terrain_image_filepaths;
	metternich::employment_type *employment_type = nullptr;
	int employment_capacity = 0;
	centesimal_int output_multiplier;
	std::vector<const terrain_type *> terrain_types; //the terrain types where the improvement can be built
	int variation_count = 1;
	improvement *required_improvement = nullptr;
	technology *required_technology = nullptr;
};

}
