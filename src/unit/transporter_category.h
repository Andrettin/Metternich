#pragma once

namespace metternich {

enum class transporter_category {
	none,
	land_transporter,
	small_merchant_ship,
	large_merchant_ship
};

}

Q_DECLARE_METATYPE(metternich::transporter_category)
