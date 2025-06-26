import QtQuick
import QtQuick.Controls

Row {
	id: scripted_modifier_row
	height: 16 * scale_factor
	spacing: 2 * scale_factor
	
	property var scope: null
	
	Repeater {
		model: scope ? scope.game_data.scripted_modifiers : []
		
		ScriptedModifierImage {
			scripted_modifier_pair: model.modelData
			scope: scripted_modifier_row.scope
		}
	}
}
