#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class icon_image_provider final : public image_provider_base
{
public:
	icon_image_provider();

	virtual void load_image(const std::string &id) override;
};

}