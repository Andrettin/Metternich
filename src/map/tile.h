#pragma once

namespace metternich {

class domain;
class province;

class tile final
{
public:
	using province_index_type = uint16_t;
	static constexpr province_index_type no_province_index = std::numeric_limits<province_index_type>::max();
	static constexpr province_index_type last_valid_province_index = no_province_index - 1;

	metternich::province *get_province() const;
	void set_province(metternich::province *province);

	const domain *get_owner() const;

private:
	province_index_type province_index = tile::no_province_index;
};

}
