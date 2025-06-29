#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class celestial_body_type final : public named_data_entry, public data_type<celestial_body_type>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path image_filepath MEMBER image_filepath WRITE set_image_filepath)
	Q_PROPERTY(int variation_count MEMBER variation_count READ get_variation_count)

public:
	static constexpr const char class_identifier[] = "celestial_body_type";
	static constexpr const char property_class_identifier[] = "metternich::celestial_body_type*";
	static constexpr const char database_folder[] = "celestial_body_types";

	explicit celestial_body_type(const std::string &identifier);
	~celestial_body_type();

	virtual void initialize() override;

	const std::filesystem::path &get_image_filepath() const
	{
		return this->image_filepath;
	}

	void set_image_filepath(const std::filesystem::path &filepath);

	int get_variation_count() const
	{
		return this->variation_count;
	}

signals:
	void changed();

private:
	std::filesystem::path image_filepath;
	int variation_count = 1;
};

}
