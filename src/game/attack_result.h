#pragma once

namespace metternich {

enum class attack_result {
	miss,
	fall_back,
	hit,
	route,
	destroy
};

}

Q_DECLARE_METATYPE(metternich::attack_result)
