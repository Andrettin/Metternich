import QtQuick
import QtQuick.Controls

CustomIconImage {
	id: modifier_icon
	name: feature.name
	description: modifier_string
	icon_identifier: feature.icon.identifier
	
	property var feature: null
	property var site: null
	readonly property string modifier_string: feature.get_modifier_string(site)
}
