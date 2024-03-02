#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class transporter_category {
	none,
	land_transporter,
	small_merchant_ship,
	large_merchant_ship
};

}

extern template class archimedes::enum_converter<metternich::transporter_category>;

Q_DECLARE_METATYPE(metternich::transporter_category)
