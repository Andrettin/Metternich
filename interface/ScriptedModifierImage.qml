import QtQuick
import QtQuick.Controls

CustomIconImage {
	id: modifier_icon
	icon_identifier: scripted_modifier.icon.identifier + "/small/" + (scripted_modifier.negative ? "red" : "green")
	tooltip: scripted_modifier.name + (modifier_string.length > 0 ? format_text(small_text("\n"
		+ "\nDuration: " + (duration * metternich.defines.months_per_turn) + " Months"
		+ "\n" + modifier_string)) : "")
	
	property var scripted_modifier_pair: null
	property var scope: null
	readonly property var scripted_modifier: scripted_modifier_pair.key
	readonly property int duration: scripted_modifier_pair.value
	readonly property string modifier_string: scripted_modifier.get_modifier_string(scope)
}
